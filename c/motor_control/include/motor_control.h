#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#include "xxmodelPan.h"
#include "xxmodelTilt.h"
#include "bus_interface.h"

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#define PITCH_RAD_PER_COUNT ((160.0 / 750.0) * (M_PI / 180.0)) // yaw 750 pulses 160 degrees
#define YAW_RAD_PER_COUNT (M_PI / 2500.0)                      // pitch 2200 pulses per 180 degrees
#define PERIOD 2500

void send_pwm_signal(double pwm_value_yaw, double pwm_value_pitch);
void read_encoder_values(void);
void home(void);
#endif