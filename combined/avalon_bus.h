#ifndef AVALON_BUS_H
#define AVALON_BUS_H

#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#include "soc_system.h"

static void *map;
static int fd = -1;

int init_bus(void);

uint32_t read_bus(void);

void write_bus(uint32_t value);

void cleanup_bus(void);

#endif /* AVALON_BUS_H */
