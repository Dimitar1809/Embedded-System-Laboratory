#include <error.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "soc_system.h"


int main(int argc, char** argv) {
    int fd = 0;

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Couldn't open /dev/mem\n");
        return -1;
    }

    uint8_t* esl_pwm_map = NULL;
    esl_pwm_map = (uint8_t*)mmap(NULL, 
                                HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_SPAN,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd,
                                HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_BASE);
    if (esl_pwm_map == MAP_FAILED) {
        perror("Couldn't map bridge.");
        close(fd);
        return -1;
    }
    volatile uint32_t *pwm_regs = (volatile uint32_t*)esl_pwm_map;

    char line[64];
    int percent_pitch, percent_yaw;
    char dir_char_pitch, dir_char_yaw;

    // --- Interactive loop ---
    while (1) {
        // Pitch speed
        do {
            printf("Enter pitch speed (0–100%%) or <Enter> to quit: ");
            if (!fgets(line, sizeof line, stdin)) goto cleanup;
            if (line[0] == '\n')  goto cleanup;
        } while (sscanf(line, "%d", &percent_pitch) != 1
                 || percent_pitch < 0
                 || percent_pitch > 100);

        // Pitch direction
        do {
            printf("Enter pitch dir [F=forward, R=reverse, B=brake, S=stop] or <Enter> to quit: ");
            if (!fgets(line, sizeof line, stdin)) goto cleanup;
            if (line[0] == '\n')  goto cleanup;
        } while (sscanf(line, " %c", &dir_char_pitch) != 1
                 || (dir_char_pitch!='F' && dir_char_pitch!='f'
                  && dir_char_pitch!='R' && dir_char_pitch!='r'
                  && dir_char_pitch!='B' && dir_char_pitch!='b'
                  && dir_char_pitch!='S' && dir_char_pitch!='s'));

        // Yaw speed
        do {
            printf("Enter yaw speed (0–100%%) or <Enter> to quit: ");
            if (!fgets(line, sizeof line, stdin)) goto cleanup;
            if (line[0] == '\n')  goto cleanup;
        } while (sscanf(line, "%d", &percent_yaw) != 1
                 || percent_yaw < 0
                 || percent_yaw > 100);

        // Yaw direction
        do {
            printf("Enter yaw dir [F=forward, R=reverse, B=brake, S=stop] or <Enter> to quit: ");
            if (!fgets(line, sizeof line, stdin)) goto cleanup;
            if (line[0] == '\n')  goto cleanup;
        } while (sscanf(line, " %c", &dir_char_yaw) != 1
                 || (dir_char_yaw!='F' && dir_char_yaw!='f'
                  && dir_char_yaw!='R' && dir_char_yaw!='r'
                  && dir_char_yaw!='B' && dir_char_yaw!='b'
                  && dir_char_yaw!='S' && dir_char_yaw!='s'));

        // 2) Convert to duty count and direction code 
        const uint16_t period = 5000;
        uint16_t duty_pitch = (uint16_t)(percent_pitch * period / 100);
        uint8_t  dir_pitch;
        switch (dir_char_pitch) {
          case 'F': case 'f': dir_pitch = 2; break;
          case 'R': case 'r': dir_pitch = 1; break;
          case 'B': case 'b': dir_pitch = 3; break;
          case 'S': case 's': dir_pitch = 0; break;
        }
        uint16_t duty_yaw = (uint16_t)(percent_yaw * period / 100);
        uint8_t  dir_yaw;
        switch (dir_char_yaw) {
          case 'F': case 'f': dir_yaw = 2; break;
          case 'R': case 'r': dir_yaw = 1; break;
          case 'B': case 'b': dir_yaw = 3; break;
          case 'S': case 's': dir_yaw = 0; break;
        }

        // 3) Build and write the command words 
        uint32_t combined_cmd =
            ((uint32_t)(duty_yaw     & 0x3FFFu) << 18)
            | ((uint32_t)(dir_yaw      & 0x3u)   << 16)
            | ((uint32_t)(duty_pitch   & 0x3FFFu) << 2)
            |  ((uint32_t)(dir_pitch    & 0x3u));


        printf("combined_cmd = 0b");
        for (int i = 31; i >= 0; --i) {
            putchar((combined_cmd >> i & 1) ? '1' : '0');
            if (i % 8 == 0 && i != 0)  putchar(' ');  // space every byte
        }
        putchar('\n');

        pwm_regs[0] = combined_cmd;
    }

cleanup:
    close(fd);
    return 0;
}

