#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include "xxmodelPan.h"   // generated header for pan motor
#include "xxmodelTilt.h"  // generated header for tilt motor

#include "soc_system.h"
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>



volatile uint16_t *raw_yaw_count;
volatile uint16_t *raw_pitch_count;
uint16_t prev_yaw_count = UINT16_MAX;
uint16_t prev_pitch_count = UINT16_MAX;

uint16_t unwrapped_yaw_count = 0;
uint16_t unwrapped_pitch_count = 0;

double yaw_angle = 0.0;
double pitch_angle = 0.0;

void unwrap_encoders(){
    uint16_t raw_yaw_count
    int yaw_diff = *raw_yaw_count - prev_yaw_count;
    int pitch_diff = *raw_pitch_count - prev_pitch_count;
    if (yaw_diff > UINT16_MAX/2) {
        yaw_diff -= UINT16_MAX; // Unwrap yaw encoder
    } else if (yaw_diff < -UINT16_MAX/2) {
        yaw_diff += UINT16_MAX; // Unwrap yaw encoder
    }
    if (pitch_diff > UINT16_MAX/2) {
        pitch_diff -= UINT16_MAX; // Unwrap pitch encoder
    } else if (pitch_diff < -UINT16_MAX/2) {
        pitch_diff += UINT16_MAX; // Unwrap pitch encoder
    }
    prev_yaw_count = *raw_yaw_count;
    prev_pitch_count = *raw_pitch_count;
    unwrapped_yaw_count += yaw_diff;
    unwrapped_pitch_count += pitch_diff;

}

// function to read the encoder value
void read_encoder_value() {

    // precompute constants
    const double YAW_RAD_PER_COUNT   = (160.0/750.0) * (M_PI/180.0); // yaw 750 pulses 160 degrees
    const double PITCH_RAD_PER_COUNT = M_PI / 2200.0; // pitch 2200 pulses per 180 degrees

    yaw_angle  = unwrapped_yaw_count   * YAW_RAD_PER_COUNT;
    pitch_angle = unwrapped_pitch_count * PITCH_RAD_PER_COUNT;
}

void send_pwm_signal(double pwm_value_yaw, double pwm_value_pitch, volatile uint32_t *regs) {
    // Function to send PWM signal

    const uint16_t PERIOD = 2500;

    uint16_t duty_yaw = (uint16_t)(fabs(pwm_value_yaw) * PERIOD);
    uint8_t  dir_yaw;
    if (pwm_value_yaw < 0) {
        dir_yaw = 1; // Reverse direction
    } else {
        dir_yaw = 2; // Forward direction
    }

    uint16_t duty_pitch = (uint16_t)(fabs(pwm_value_pitch) * PERIOD);
    uint8_t  dir_pitch;
    if (pwm_value_pitch < 0) {
        dir_pitch = 2; // Reverse direction
    } else {
        dir_pitch = 1; // Forward direction
    }

    uint32_t cmd = ((uint32_t)(duty_yaw   & 0x3FFFu) << 18)
                  | ((uint32_t)(dir_yaw    & 0x03u)   << 16)
                  | ((uint32_t)(duty_pitch & 0x3FFFu) <<  2)
                  | ((uint32_t)(dir_pitch  & 0x03u));
    regs[0] = cmd;
}

static volatile int keep_running = 1;
void handle_sigint(int sig) { keep_running = 0; }


void home(volatile uint32_t *regs) {
    printf("Homing motors...\n");

    // 1) initial shove toward home
    send_pwm_signal(-0.2, 0.2, regs);
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 500000000 };
    nanosleep(&ts, NULL);

    // 2) loop until counts are zero or no longer change
    printf("Waiting for motors to reach home position...\n");
    uint16_t prev_yaw = UINT16_MAX, prev_pitch = UINT16_MAX;
    for (;;) {
        unwrap_encoders();
        read_encoder_value();
        

        // done when both axes hit zero OR counts have stalled
        if ((yaw == 0 && pitch == 0) ||
            (yaw == prev_yaw && pitch == prev_pitch)) {
            break;
        }

        prev_yaw   = yaw;
        prev_pitch = pitch;

        send_pwm_signal(-0.2, 0.2, regs);
        ts.tv_nsec = 10000000;  // 10 ms
        nanosleep(&ts, NULL);
    }

    // 3) stop motors and report
    send_pwm_signal(0.0, 0.0, regs);
    printf("Motors homed successfully.\n");
}

int main(void) {

    // Initalize buffer to send the PWM signals and read the encoder values
    int fd  = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd < 0) {
        perror("open(/dev/mem)");
        return 1;
    }

    void *map = mmap(NULL,
                     HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_SPAN,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd,
                     HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_BASE);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    // Assign global pointers:
    raw_yaw_count = (volatile uint16_t *)map;
    raw_pitch_count = ((volatile uint16_t *)map) + 1;



    // Catch Ctrl+C
    signal(SIGINT, handle_sigint);  


    home(); // Home the motors before starting
    // Initialize the models once
    pan_XXModelInitialize();
    tilt_XXModelInitialize();
    
    // Ask for desired positions
    double desired_position_pan;
    double desired_position_tilt;
    printf("Enter desired pan position [rad] and tilt position [rad], separated by space: ");
    if (scanf("%lf %lf", &desired_position_pan, &desired_position_tilt) != 2) {
       fprintf(stderr, "Invalid input. Please enter two numbers. Exiting.\n");
        return 1;
    }

    // Prepare fixed timestep sleep
    struct timespec ts;
    double dt = pan_xx_step_size; // might not work, so change to 0.01 if needed
    ts.tv_sec = (time_t)dt;
    ts.tv_nsec = (long)((dt - ts.tv_sec) * 1e9);

    printf("\nStarting real-time control loop (Ctrl+C to stop)...\n\n");

    // Real-time loop
    while (keep_running) {
        // read the encoder value
        angles_t encoder_values = read_encoder_value(regs);

        // feed the model inputs
        pan_xx_V[7] = desired_position_pan;
        pan_xx_V[8] = encoder_values.yaw; // pan angle
        tilt_xx_V[9] = desired_position_tilt;
        tilt_xx_V[10] = encoder_values.pitch; // tilt angle

        // One control step
        pan_XXCalculateDynamic();
        pan_XXCalculateOutput();
        tilt_XXCalculateDynamic();
        tilt_XXCalculateOutput();

        // Send the PWM signal
        send_pwm_signal(pan_xx_V[9], tilt_xx_V[11], regs); // Assuming xx_V[9]/[11] is the PWM output
        printf("Pan: %.2f rad, Tilt: %.2f rad, PWM Pan: %.2f, PWM Tilt: %.2f\n",
               encoder_values.yaw, encoder_values.pitch,
               pan_xx_V[9], tilt_xx_V[11]);
        // wait dt seconds
        nanosleep(&ts, NULL);
    }

    pan_XXModelTerminate();
    tilt_XXModelTerminate();
    munmap((void*)regs, HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_SPAN);
    close(fd);
    printf("\nTerminating control loop.\n");
    return 0;
}