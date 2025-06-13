#include "motor_control.h"

uint16_t prev_yaw_count = UINT16_MAX;
uint16_t prev_pitch_count = UINT16_MAX;

int16_t unwrapped_yaw_count;
int16_t unwrapped_pitch_count;
double yaw_angle;
double pitch_angle;

double pwm_limiter_yaw = 0.5;   // Limit the maximum PWM value to prevent saturation
double pwm_limiter_pitch = 0.1; // Limit the maximum PWM value to prevent saturation

void send_pwm_signal(double pwm_value_yaw, double pwm_value_pitch)
{
    pwm_value_pitch = pwm_value_pitch * pwm_limiter_pitch;
    uint16_t duty_yaw = (uint16_t)(fabs(pwm_value_yaw) * PERIOD);
    uint8_t dir_yaw;
    if (pwm_value_yaw < 0)
        dir_yaw = 2; // Reverse direction
    else
        dir_yaw = 1; // Forward direction

    uint16_t duty_pitch = (uint16_t)(fabs(pwm_value_pitch) * PERIOD);
    uint8_t dir_pitch;
    if (pwm_value_pitch < 0)
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
    int16_t yaw_diff = prev_yaw_count - yaw_count;
    int16_t pitch_diff = prev_pitch_count - pitch_count;

    prev_yaw_count = yaw_count;
    prev_pitch_count = pitch_count;

    if (yaw_diff > UINT16_MAX / 2)
        yaw_diff -= UINT16_MAX;
    else if (yaw_diff < -UINT16_MAX / 2)
        yaw_diff += UINT16_MAX;

    if (pitch_diff > UINT16_MAX / 2)
        pitch_diff -= UINT16_MAX;
    else if (pitch_diff < -UINT16_MAX / 2)
        pitch_diff += UINT16_MAX;

    unwrapped_yaw_count += yaw_diff;
    unwrapped_pitch_count += pitch_diff;

    yaw_angle = unwrapped_yaw_count * YAW_RAD_PER_COUNT;
    pitch_angle = unwrapped_pitch_count * PITCH_RAD_PER_COUNT;
}

void home(void)
{
    double prev_yaw_angle = 0;
    double prev_pitch_angle = 0;
    int stable_count = 0;
    const int STABLE_THRESHOLD = 5;         // Number of consecutive readings with no change
    const double PWM_HOME_SPEED_YAW = -1;   // Negative for backwards direction
    const double PWM_HOME_SPEED_PITCH = -1; // Negative for backwards direction

    printf("Starting homing sequence...\n");

    // Read initial encoder values
    read_encoder_values();
    prev_yaw_angle = yaw_angle;
    prev_pitch_angle = pitch_angle;

    while (stable_count < STABLE_THRESHOLD)
    {
        // Send PWM signal to move backwards
        send_pwm_signal(PWM_HOME_SPEED_YAW, PWM_HOME_SPEED_PITCH);

        // Small delay to allow movement
        struct timespec delay = {.tv_sec = 0, .tv_nsec = 10000000}; // 10ms
        nanosleep(&delay, NULL);

        // Read current encoder values
        read_encoder_values();

        // Check if angles have stopped changing
        if (fabs(yaw_angle - prev_yaw_angle) < 0.001 &&
            fabs(pitch_angle - prev_pitch_angle) < 0.001)
        {
            stable_count++;
        }
        else
        {
            stable_count = 0; // Reset if movement detected
        }

        prev_yaw_angle = yaw_angle;
        prev_pitch_angle = pitch_angle;
    }

    // Stop motors
    send_pwm_signal(0.0, 0.0);

    // Reset encoder counts to zero at home position
    unwrapped_yaw_count = 0;
    unwrapped_pitch_count = 0;
    yaw_angle = 0.0;
    pitch_angle = 0.0;

    printf("Homing complete. Motors at home position.\n");
}

void position_to_angle(uint16_t x, uint16_t y, double *desired_angle_x, double *desired_angle_y) {
    //Image dimensions
    const double img_w = 320.0;        // width in px
    const double img_h = 240.0;        // height in px
    const double deg_rad = M_PI/180.0; // degrees to radians conversion factor
    const double FOV = 55.0 * deg_rad; // field of view in degrees
    const double ar = img_w / img_h;   // 

    // Compute horizontal & vertical FoV from diagonal FoV
    const double hFOV = 2.0 * atan( tan(FOV/2.0) * (ar / sqrt(1 + ar*ar)) );
    const double vFOV = 2.0 * atan( tan(FOV/2.0) * (1.0/    sqrt(1 + ar*ar)) );

    // Pixel offsets from center
    const double dx = x - (img_w / 2.0); // + right
    const double dy = y - (img_h / 2.0); // + up (invert Y if needed)
    
    // Radians per pixel
    const double rad_per_px_x = hFOV / img_w;
    const double rad_per_px_y = vFOV / img_h;

    double angle_x = dx * rad_per_px_x; // pan 
    double angle_y = dy * rad_per_px_y; // tilt
    printf("Converted angles, pan: %f, tilt: %f)\n", angle_x, angle_y);
    *desired_angle_x = angle_x; // Store pan angle
    *desired_angle_y = angle_y; // Store tilt angle
}

// Rerequired to prompt the user for desired angle
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

    // Ask for desired positions
    double desired_position_pan = 0;
    double desired_position_tilt = 0;
    // printf("Enter desired pan position [rad] and tilt position [rad], separated by space: ");
    // if (scanf("%lf %lf", &desired_position_pan, &desired_position_tilt) != 2)
    // {
    //     fprintf(stderr, "Invalid input. Please enter two numbers. Exiting.\n");
    //     return 1;
    // }

    // Prepare fixed timestep sleep
    struct timespec ts;
    double dt = pan_xx_step_size; // might not work, so change to 0.01 if needed
    ts.tv_sec = (time_t)dt;
    ts.tv_nsec = (long)((dt - ts.tv_sec) * 1e9);

    printf("\nStarting real-time control loop (Ctrl+C to stop)...\n\n");

    // Real-time loop
    while (keep_running)
    {

        int ball_x, ball_y;
    
        if (has_new_frame() && get_ball_position(&ball_x, &ball_y)) {
            printf("Ball at (%d, %d)\n", ball_x, ball_y);
	    position_to_angle(ball_x, ball_y, &desired_position_pan, &desired_position_tilt);
        }

        read_encoder_values();

        // Feed the model inputs
        pan_xx_V[7] = desired_position_pan;
        pan_xx_V[8] = yaw_angle; // pan angle
        tilt_xx_V[9] = desired_position_tilt;
        tilt_xx_V[10] = pitch_angle; // tilt angle

        // One control step
        pan_XXCalculateDynamic();
        pan_XXCalculateOutput();
        tilt_XXCalculateDynamic();
        tilt_XXCalculateOutput();

        // Send PWM signal to motors
        send_pwm_signal(pan_xx_V[9], tilt_xx_V[11]);
        // printf("Pan: %.2f rad, Tilt: %.2f rad, PWM Pan: %.2f, PWM Tilt: %.2f\n",
        //        yaw_angle, pitch_angle,
        //        pan_xx_V[9], tilt_xx_V[11]);

        // Sleep for fixed timestep
        nanosleep(&ts, NULL);
    }

    // Cleanup
    cleanup_bus();
    image_processing_stop();
    printf("\nTerminating control loop.\n");
    return 0;
}
