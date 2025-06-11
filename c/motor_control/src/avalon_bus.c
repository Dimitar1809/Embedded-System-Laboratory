#include "bus_interface.h"
#include "soc_system.h"
#include <fcntl.h>        // for open(), O_RDWR, O_SYNC
#include <sys/mman.h>     // for mmap(), munmap(), MAP_SHARED, MAP_FAILED, PROT_READ, PROT_WRITE
#include <unistd.h>       // for close()
#include <stdio.h>        // for perror()
#include <stdint.h>       // for uint32_t (though this comes from bus_interface.h)


static void *map;
static int fd = -1;

int init_bus(void)
{
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        perror("open(/dev/mem)");
        return 1;
    }

    map = mmap(NULL,
               HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_SPAN,
               PROT_READ | PROT_WRITE,
               MAP_SHARED, fd,
               HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_BASE);
    if (map == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        fd = -1;
        return 1;
    }
    return 0;
}

uint32_t read_bus(void)
{
    return *(volatile uint32_t *)map;
}

void write_bus(uint32_t value)
{
    *(volatile uint32_t *)map = value;
}

void cleanup_bus(void)
{
    if (map != MAP_FAILED && map != NULL)
    {
        munmap(map, HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_SPAN);
        map = NULL;
    }

    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}
