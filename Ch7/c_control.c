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

typedef struct {
    double yaw;
    double pitch;
} angles_t;

// function to read the encoder value
angles_t read_encoder_value(volatile uint32_t *regs) {

    // precompute constants
    const double YAW_RAD_PER_COUNT   = (160.0/750.0) * (M_PI/180.0); // yaw 750 pulses 160 degrees
    const double PITCH_RAD_PER_COUNT = M_PI / 2200.0; // pitch 2200 pulses per 180 degrees

    // read the encoder values from the hardware registers
    uint32_t combined = regs[0];
    uint16_t yaw_enc = (uint16_t)(combined & 0x0000FFFF);
    uint16_t pitch_enc = (uint16_t)((combined >> 16) & 0x0000FFFF);

    // after reading your uint16_t yaw_value, pitch_value:
    angles_t angles;
    angles.yaw   = yaw_enc   * YAW_RAD_PER_COUNT;
    angles.pitch = pitch_enc * PITCH_RAD_PER_COUNT;
    return angles;
}

void send_pwm_signal(double pwm_value_yaw, double pwm_value_pitch, volatile uint32_t *regs) {
    // Function to send PWM signal

    const uint16_t PERIOD = 5000;

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
        dir_pitch = 1; // Reverse direction
    } else {
        dir_pitch = 2; // Forward direction
    }

    uint32_t cmd = ((uint32_t)(duty_yaw   & 0x3FFFu) << 18)
                  | ((uint32_t)(dir_yaw    & 0x03u)   << 16)
                  | ((uint32_t)(duty_pitch & 0x3FFFu) <<  2)
                  | ((uint32_t)(dir_pitch  & 0x03u));
    regs[0] = cmd;
}

static volatile int keep_running = 1;
void handle_sigint(int sig) { keep_running = 0; }

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
    volatile uint32_t *regs = map;

    // Catch Ctrl+C
    signal(SIGINT, handle_sigint);  

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