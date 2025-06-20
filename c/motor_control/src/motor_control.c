#include "motor_control.h"

#define IMAGE_WIDTH 320
#define IMAGE_HEIGHT 240
#define FOV (55 * M_PI / 180.0) // Field of view in radians
#define HFOV (45 * M_PI / 180.0) // Horizontal field of view in radians
#define VFOV (34 * M_PI / 180.0) // Vertical field of view in radians

#define TAU_PAN 1 // Time constant for pan (adjust as needed)
#define TAU_TILT 1 // Time constant for tilt (adjust as needed)

#define CONTROLLER_PERIOD 0.01 // Controller period in seconds
#define CONTROLLER_PERIOD_NS 10000000 // 10ms in nanoseconds

#define PITCH_PWM_MULTIPLIER 0.05// Multiplier for pitch PWM value

uint16_t prev_yaw_count = UINT16_MAX;
uint16_t prev_pitch_count = UINT16_MAX;
int16_t unwrapped_yaw_count;
int16_t unwrapped_pitch_count;

double yaw_angle;
double pitch_angle;

double yaw_target_position = 0.0;
double pitch_target_position = 0.0;
double yaw_target_position_raw = 0.0;
double pitch_target_position_raw = 0.0;



void send_pwm_signal(double yaw_pwm_value, double pitch_pwm_value) 
{
    pitch_pwm_value = pitch_pwm_value * PITCH_PWM_MULTIPLIER; // Scale pitch PWM value
    printf("Sending PWM signal: Yaw: %.2f, Pitch: %.2f\n", yaw_pwm_value, pitch_pwm_value);
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

    yaw_angle = unwrapped_yaw_count * YAW_RAD_PER_COUNT;
    pitch_angle = unwrapped_pitch_count * PITCH_RAD_PER_COUNT;

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
	double prev_yaw_angle = yaw_angle;
	double prev_pitch_angle = pitch_angle;

    send_pwm_signal(-1, -1);
    
	struct timespec next;
	clock_gettime(CLOCK_MONOTONIC, &next);

	uint8_t stable_count = 0;
    while (stable_count < 100)
    {
        read_encoder_values();

		if (fabs(yaw_angle - prev_yaw_angle) < 0.001 && fabs(pitch_angle - prev_pitch_angle) < 0.001)
			stable_count++;
		else
			stable_count = 0;

        prev_yaw_angle = yaw_angle;
        prev_pitch_angle = pitch_angle;

		next.tv_nsec += CONTROLLER_PERIOD_NS;
        next.tv_sec  += next.tv_nsec / 1000000000;
        next.tv_nsec %= 1000000000;

		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    send_pwm_signal(0.0, 0.0);

    yaw_angle = 0.0;
    pitch_angle = 0.0;

    unwrapped_yaw_count = 0;
    unwrapped_pitch_count = 0;

    printf("Homing complete. Motors at home position.\n");
}

void update_target_position(void)
{
	int ball_x, ball_y;
	if (!has_new_frame()) {
  		return;
	}
	if (!get_ball_position(&ball_x, &ball_y)) {
		return;
 	}
	
	double dx_angle, dy_angle;
	position_to_angle(ball_x, ball_y, &dx_angle, &dy_angle);
	yaw_target_position_raw += dx_angle;
	pitch_target_position_raw += dy_angle;

	// Ensure desired positions are within limits
	if (yaw_target_position_raw <  0) yaw_target_position_raw = 0;
	if (yaw_target_position_raw > M_PI) yaw_target_position_raw = M_PI;
	if (pitch_target_position_raw < 0) pitch_target_position_raw = 0;
	if (pitch_target_position_raw > 2.79) pitch_target_position_raw = 2.79;

}

// …existing code…

// PID gains for smoothing pan
#define KP_PAN  5.0
#define KI_PAN  0.1
#define KD_PAN  0.5
// PID gains for smoothing tilt
#define KP_TILT 5.0
#define KI_TILT 0.1
#define KD_TILT 0.5

// clamp helper
static inline double clamp(double v, double lo, double hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

void smoothen_target_position(void)
{
    // static states for pan
    static double pan_integral   = 0.0;
    static double pan_prev_error = 0.0;
    // static states for tilt
    static double tilt_integral   = 0.0;
    static double tilt_prev_error = 0.0;

    // compute errors
    double err_pan  = yaw_target_position_raw   - yaw_target_position;
    double err_tilt = pitch_target_position_raw - pitch_target_position;

    // integrate
    pan_integral  += err_pan  * CONTROLLER_PERIOD;
    tilt_integral += err_tilt * CONTROLLER_PERIOD;
    // (optional) clamp integrator to avoid windup:
    pan_integral  = clamp(pan_integral,  -1.0, 1.0);
    tilt_integral = clamp(tilt_integral, -1.0, 1.0);

    // derivative
    double d_pan  = (err_pan  - pan_prev_error)  / CONTROLLER_PERIOD;
    double d_tilt = (err_tilt - tilt_prev_error) / CONTROLLER_PERIOD;

    // PID outputs (rad/s)
    double u_pan  = KP_PAN  * err_pan
                  + KI_PAN  * pan_integral
                  + KD_PAN  * d_pan;
    double u_tilt = KP_TILT * err_tilt
                  + KI_TILT * tilt_integral
                  + KD_TILT * d_tilt;

    // update for next step
    pan_prev_error  = err_pan;
    tilt_prev_error = err_tilt;

    // update smoothed positions
    yaw_target_position   += u_pan  * CONTROLLER_PERIOD;
    pitch_target_position += u_tilt * CONTROLLER_PERIOD;

    // clamp final smoothed positions to your physical limits
    yaw_target_position   = clamp(yaw_target_position,   0.0, M_PI);
    pitch_target_position = clamp(pitch_target_position, 0.0, 2.79);
}

// …existing code…

void position_to_angle(uint16_t x, uint16_t y, double *dx_angle, double *dy_angle) {
    int pixel_error_x = x - (IMAGE_WIDTH / 2); // X_CENTER is the center of the image in pixels
    int pixel_error_y = y - (IMAGE_HEIGHT / 2); // Y_CENTER is the center of the image in pixels
    double angular_error_x = (double) pixel_error_x / IMAGE_WIDTH * HFOV;
    double angular_error_y = (double) pixel_error_y / IMAGE_HEIGHT * VFOV;
    printf("Pixel error, x: %d, y: %d\n", pixel_error_x, pixel_error_y);
    printf("Angular error, x: %f, y: %f\n", angular_error_x, angular_error_y);
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

    // Real-time loop
    while (keep_running)
    {
        // print time
        printf("Current time: %ld.%09ld\n", next.tv_sec, next.tv_nsec);
		update_target_position();
        
		smoothen_target_position();

        printf("Yaw target position: %.2f rad, Pitch target position: %.2f rad\n",
               yaw_target_position, pitch_target_position);

        read_encoder_values();

        printf("Yaw angle: %.2f rad, Pitch angle: %.2f rad\n", yaw_angle, pitch_angle);

        // Feed the model inputs
        pan_xx_V[7] = yaw_target_position;
        pan_xx_V[8] = yaw_angle; // pan angle
        tilt_xx_V[9] = pitch_target_position;
        tilt_xx_V[10] = pitch_angle; // tilt angle

        // One control step
        pan_XXCalculateDynamic();
        pan_XXCalculateOutput();
        tilt_XXCalculateDynamic();
        tilt_XXCalculateOutput();

        // Send PWM signal to motors
        send_pwm_signal(pan_xx_V[9], tilt_xx_V[11]);

        next.tv_nsec += CONTROLLER_PERIOD_NS;
        next.tv_sec  += next.tv_nsec / 1000000000;
        next.tv_nsec %= 1000000000;

        printf("\n");
        // sleep until that time (avoids accumulating drift)
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    // Cleanup
    cleanup_bus();
    image_processing_stop();
    printf("\nTerminating control loop.\n");
    return 0;
}
