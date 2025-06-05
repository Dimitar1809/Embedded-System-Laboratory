#ifndef SPI_BUS_H
#define SPI_BUS_H

#include <stdint.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

void init_bus(void);
void write_bus(uint8_t *data, uint16_t length);
void read_bus(uint8_t *data, uint16_t length);

#endif // SPI_BUS_H