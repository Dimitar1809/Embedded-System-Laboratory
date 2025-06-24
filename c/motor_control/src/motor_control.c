#include "motor_control.h"

#define IMAGE_WIDTH 320
#define IMAGE_HEIGHT 240
#define FOV (55 * M_PI / 180.0) // Field of view in radians
#define HFOV (45 * M_PI / 180.0) // Horizontal field of view in radians
#define VFOV (34 * M_PI / 180.0) // Vertical field of view in radians

#define CONTROLLER_PERIOD 0.01 // Controller period in seconds
#define CONTROLLER_PERIOD_NS 10000000 // 10ms in nanoseconds

#define PITCH_PWM_MULTIPLIER 0.05// Multiplier for pitch PWM value

uint16_t prev_yaw_count = UINT16_MAX;
uint16_t prev_pitch_count = UINT16_MAX;
int16_t unwrapped_yaw_count;
int16_t unwrapped_pitch_count;

double yaw_position;
double pitch_position;

double yaw_target_position = 0.0;
double pitch_target_position = 0.0;
double yaw_target_position_raw = 0.0;
double pitch_target_position_raw = 0.0;

int ball_x, ball_y, ball_detected;

void send_pwm_signal(double yaw_pwm_value, double pitch_pwm_value) 
{
    pitch_pwm_value = pitch_pwm_value * PITCH_PWM_MULTIPLIER; // Scale pitch PWM value
    uint16_t duty_yaw = (uint16_t)(fabs(yaw_pwm_value) * PERIOD);
    uint8_t dir_yaw;
    if (yaw_pwm_value < 0)
        dir_yaw = 2; // Reverse direction
    else
        dir_yaw = 1; // Forward direction

    
    uint16_t duty_pitch = (uint16_t)(fabs(pitch_pwm_value) * PERIOD);
    uint8_t dir_pitch;
    if (pitch_pwm_value < 0)
        dir_pitch = 2; // Reverse direction
    else
        dir_pitch = 1; // Forward direction

    uint32_t cmd = ((uint32_t)(duty_yaw & 0x3FFFu) << 18) | ((uint32_t)(dir_yaw & 0x03u) << 16) | ((uint32_t)(duty_pitch & 0x3FFFu) << 2) | ((uint32_t)(dir_pitch & 0x03u));

    write_bus(cmd);
}

void read_encoder_values(void)
{
    uint32_t encoder_values = read_bus();
    uint16_t pitch_count = (encoder_values >> 16) & 0xFFFFu;
    uint16_t yaw_count = encoder_values & 0xFFFFu;

	int16_t yaw_delta = calculate_delta_wrapped(prev_yaw_count, yaw_count);
	int16_t pitch_delta = calculate_delta_wrapped(prev_pitch_count, pitch_count);

    unwrapped_yaw_count += yaw_delta;
    unwrapped_pitch_count += pitch_delta;

    yaw_position = unwrapped_yaw_count * YAW_RAD_PER_COUNT;
    pitch_position = unwrapped_pitch_count * PITCH_RAD_PER_COUNT;

	prev_yaw_count = yaw_count;
    prev_pitch_count = pitch_count;
}

static inline int16_t calculate_delta_wrapped(uint16_t prev_count, uint16_t new_count) {
	int32_t delta = prev_count - new_count;
	if (delta > INT16_MAX) {
		delta -= (UINT16_MAX + 1);
	} else if (delta < INT16_MIN) {
		delta += (UINT16_MAX + 1);
	}
	return (int16_t)delta;
}

void home(void)
{
	printf("Homing motors...\n");
	read_encoder_values();
	double prev_yaw_angle = yaw_position;
	double prev_pitch_angle = pitch_position;

    send_pwm_signal(-1, -1);
    
	struct timespec next;
	clock_gettime(CLOCK_MONOTONIC, &next);

	uint8_t stable_count = 0;
    while (stable_count < 100)
    {
        read_encoder_values();

		if (fabs(yaw_position - prev_yaw_angle) < 0.01 && fabs(pitch_position - prev_pitch_angle) < 0.01)
			stable_count++;
		else
			stable_count = 0;

        prev_yaw_angle = yaw_position;
        prev_pitch_angle = pitch_position;

		next.tv_nsec += CONTROLLER_PERIOD_NS;
        next.tv_sec  += next.tv_nsec / 1000000000;
        next.tv_nsec %= 1000000000;

		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    send_pwm_signal(0.0, 0.0);

    yaw_position = 0.0;
    pitch_position = 0.0;

    unwrapped_yaw_count = 0;
    unwrapped_pitch_count = 0;

    printf("Homing complete. Motors at home position.\n");
}

// clamp helper
static inline double clamp(double v, double lo, double hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

void update_target_position(void)
{

	if (!has_new_frame()) {
  		return;
	}
    ball_detected = get_ball_position(&ball_x, &ball_y);
	if (!ball_detected) {
		return;
 	}
	
	double dx_angle, dy_angle;
	position_to_angle(ball_x, ball_y, &dx_angle, &dy_angle);
	yaw_target_position_raw += dx_angle;
	pitch_target_position_raw += dy_angle;


	// Ensure desired positions are within limits
	yaw_target_position_raw = clamp(yaw_target_position_raw, 0.0, M_PI);
	pitch_target_position_raw = clamp(pitch_target_position_raw, 0.0, 2.79);
}

// PID gains for smoothing pan
#define KP_PAN  5.0
#define KI_PAN  0.1
#define KD_PAN  0.5
// PID gains for smoothing tilt
#define KP_TILT 5.0
#define KI_TILT 0.1
#define KD_TILT 0.5

void smoothen_target_position(void)
{
    static double pan_integral   = 0.0;
    static double pan_prev_error = 0.0;
    static double tilt_integral   = 0.0;
    static double tilt_prev_error = 0.0;

    double err_pan  = yaw_target_position_raw   - yaw_target_position;
    double err_tilt = pitch_target_position_raw - pitch_target_position;

    pan_integral  += err_pan  * CONTROLLER_PERIOD;
    tilt_integral += err_tilt * CONTROLLER_PERIOD;
    
    pan_integral  = clamp(pan_integral,  -1.0, 1.0);
    tilt_integral = clamp(tilt_integral, -1.0, 1.0);

    double d_pan  = (err_pan  - pan_prev_error)  / CONTROLLER_PERIOD;
    double d_tilt = (err_tilt - tilt_prev_error) / CONTROLLER_PERIOD;

    // PID outputs (rad/s)
    double u_pan  = KP_PAN  * err_pan
                  + KI_PAN  * pan_integral
                  + KD_PAN  * d_pan;
    double u_tilt = KP_TILT * err_tilt
                  + KI_TILT * tilt_integral
                  + KD_TILT * d_tilt;

    pan_prev_error  = err_pan;
    tilt_prev_error = err_tilt;

    // upddate target positions
    yaw_target_position   += u_pan  * CONTROLLER_PERIOD;
    pitch_target_position += u_tilt * CONTROLLER_PERIOD;

    // clamp to physical limits
    yaw_target_position   = clamp(yaw_target_position,   0.0, M_PI);
    pitch_target_position = clamp(pitch_target_position, 0.0, 2.79);
}

void position_to_angle(uint16_t x, uint16_t y, double *dx_angle, double *dy_angle) {
    int pixel_error_x = x - (IMAGE_WIDTH / 2); // X_CENTER is the center of the image in pixels
    int pixel_error_y = y - (IMAGE_HEIGHT / 2); // Y_CENTER is the center of the image in pixels
    double angular_error_x = (double) pixel_error_x / IMAGE_WIDTH * HFOV;
    double angular_error_y = (double) pixel_error_y / IMAGE_HEIGHT * VFOV;

    *dx_angle = angular_error_x;
    *dy_angle = angular_error_y;
}

static volatile int keep_running = 1;
void handle_sigint(int sig) { keep_running = 0; }

int main(void)
{
    init_bus();

    printf("starting image processing...\n");
    if (image_processing_start() != 0)
    {
        fprintf(stderr, "Failed to start image processing. Exiting.\n");
        return 1;
    }

    home();

    // Catch Ctrl+C
    signal(SIGINT, handle_sigint);

    // Initialize the models once
    pan_XXModelInitialize();
    tilt_XXModelInitialize();

    printf("\nStarting real-time control loop (Ctrl+C to stop)...\n\n");
    
    struct timespec next;
	clock_gettime(CLOCK_MONOTONIC, &next);

    int print_counter = 0;
    const int PRINT_INTERVAL = 20; // Print every 20 iterations (200ms at 10ms per iteration)


    // Real-time loop
    struct timespec loop_start, loop_end;
    double min_loop_time = 1.0, max_loop_time = 0.0, total_loop_time = 0.0;
    int loop_count = 0;
    
    while (keep_running)
    {
        clock_gettime(CLOCK_MONOTONIC, &loop_start);

        // Update target position based on image processing
        update_target_position();
         
        // Smoothen the target position using PID control
        smoothen_target_position();

         // Read encoder values
        read_encoder_values();

        // Feed the model inputs
        pan_xx_V[7] = yaw_target_position;
        pan_xx_V[8] = yaw_position; // pan angle
        tilt_xx_V[9] = pitch_target_position;
        tilt_xx_V[10] = pitch_position; // tilt angle

        // One control step
        pan_XXCalculateDynamic();
        pan_XXCalculateOutput();
        tilt_XXCalculateDynamic();
        tilt_XXCalculateOutput();

        // Send PWM signal to motors
        send_pwm_signal(pan_xx_V[9], tilt_xx_V[11]);

        clock_gettime(CLOCK_MONOTONIC, &loop_end);
        
        // Calculate loop execution time
        double loop_time = (loop_end.tv_sec - loop_start.tv_sec) + 
                          (loop_end.tv_nsec - loop_start.tv_nsec) / 1000000000.0;
        
        total_loop_time += loop_time;
        loop_count++;
        if (loop_time < min_loop_time) min_loop_time = loop_time;
        if (loop_time > max_loop_time) max_loop_time = loop_time;

        // Print status only 5 times per second (every 200ms)
        if (++print_counter >= PRINT_INTERVAL) {
            double avg_loop_time = total_loop_time / loop_count;
            printf(" [Control] avg=%.4fms, min=%.4fms| ",
                   avg_loop_time * 1000, min_loop_time * 1000);
            printf("Yaw: pos=%.2f, target=%.2f, PWM=%.2f | "
                   "Pitch: pos=%.2f, target=%.2f, PWM=%.2f | ",
                   yaw_position, yaw_target_position, pan_xx_V[9],
                   pitch_position, pitch_target_position, tilt_xx_V[11]);
            if (ball_detected) {
                printf("Ball (%d, %d)\n", ball_x, ball_y);
            } else {
                printf("No ball \n");
            }
            print_counter = 0;
            
            // Reset timing stats periodically
            total_loop_time = 0.0;
            loop_count = 0;
            min_loop_time = 1.0;
            max_loop_time = 0.0;
        }

        next.tv_nsec += CONTROLLER_PERIOD_NS;
        next.tv_sec  += next.tv_nsec / 1000000000;
        next.tv_nsec %= 1000000000;

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    // Cleanup
    cleanup_bus();
    image_processing_stop();
    printf("\nTerminating control loop.\n");
    return 0;
}
