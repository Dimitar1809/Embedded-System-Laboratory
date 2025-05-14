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
    uint8_t* esl_demo_map_base = NULL; // Renamed for clarity
    esl_demo_map_base = (uint8_t*)mmap(NULL, HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_BASE);
    if (esl_demo_map_base == MAP_FAILED) {
        perror("Couldn't map bridge.");
        close(fd);
        return -1;
    }

    // Define the address for the combined 32-bit counter value
    // This points to the base address of your peripheral (offset 0x00)
    volatile uint32_t* addr_combined_counters = (volatile uint32_t*)(esl_demo_map_base);

    // Continuous reading loop
    while (1) {
        uint32_t combined_value = *addr_combined_counters; // Read the single 32-bit value
        
        // Extract the 16-bit values
        // Assuming yaw is in the lower 16 bits and pitch is in the upper 16 bits
        uint16_t yaw_value = (uint16_t)(combined_value & 0x0000FFFF);         // Mask lower 16 bits
        uint16_t pitch_value = (uint16_t)((combined_value >> 16) & 0x0000FFFF); // Shift right by 16, then mask

        printf("Yaw Counter: %u 	Pitch Counter: %u \n", 
               yaw_value, pitch_value
               );

        usleep(500000); // Sleep for 100ms to avoid excessive CPU usage
    }

    // This part is unreachable due to the while(1) loop
    // Consider adding a signal handler to break the loop and clean up
    munmap(esl_demo_map_base, HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_SPAN); // Unmap memory
    // *((uint32_t *)esl_demo_map_base) = 1 << 31 | 0x08; // This line's purpose is unclear
    close(fd);
    return 0;
}