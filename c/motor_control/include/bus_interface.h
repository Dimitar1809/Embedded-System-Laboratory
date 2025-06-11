#ifndef BUS_INTERFACE_H
#define BUS_INTERFACE_H

#include <stdint.h>

int init_bus(void);

uint32_t read_bus(void);

void write_bus(uint32_t value);

void cleanup_bus(void);

#endif /* BUS_INTERFACE_H */
