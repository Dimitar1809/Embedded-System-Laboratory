#include "bus_interface.h"
#include <fcntl.h>        // for open()
#include <unistd.h>       // for close()
#include <stdio.h>        // for sprintf()
#include <string.h>       // for memset(), memcpy()
#include <sys/ioctl.h>    // for ioctl()
#include <linux/spi/spidev.h>
#include <linux/types.h>

int speed = 10000000; // Default speed
int fd = -1;

#define MAX_SPI_BUFSIZ 8096
char RXBuf[MAX_SPI_BUFSIZ];
char TXBuf[MAX_SPI_BUFSIZ];

uint32_t last_written_value = 0; // Track last written value

int init_bus(void)
{
    unsigned spiChan = 1; // Default to channel 1 like in main.c
    char spiMode;
    char spiBits;
    char dev[32];

    spiMode = (0 & 3);
    spiBits = 8;

    sprintf(dev, "/dev/spidev0.%d", spiChan);

    if ((fd = open(dev, O_RDWR)) < 0)
    {
        return 1; // Match avalon_bus error return
    }

    if (ioctl(fd, SPI_IOC_WR_MODE, &spiMode) < 0)
    {
        close(fd);
        fd = -1;
        return 1;
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spiBits) < 0)
    {
        close(fd);
        fd = -1;
        return 1;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
    {
        close(fd);
        fd = -1;
        return 1;
    }

    return 0; // Success
}

uint32_t read_bus(void)
{
    uint32_t tx_word = last_written_value; // Send last written value for read
    uint32_t rx_word = 0;
    int err;
    struct spi_ioc_transfer spi;

    memset(&spi, 0, sizeof(spi));

    // Pack 32-bit word into TXBuf
    memcpy(TXBuf, &tx_word, sizeof(tx_word));

    spi.tx_buf = (unsigned long)TXBuf;
    spi.rx_buf = (unsigned long)RXBuf;
    spi.len = sizeof(tx_word);
    spi.speed_hz = speed;
    spi.delay_usecs = 0;
    spi.bits_per_word = 8;
    spi.cs_change = 0;

    err = ioctl(fd, SPI_IOC_MESSAGE(1), &spi);

    // Unpack returned 32-bit word
    memcpy(&rx_word, RXBuf, sizeof(rx_word));

    return rx_word;
}

void write_bus(uint32_t value)
{
    uint32_t rx_word = 0;
    int err;
    struct spi_ioc_transfer spi;

    memset(&spi, 0, sizeof(spi));

    // Pack 32-bit word into TXBuf
    memcpy(TXBuf, &value, sizeof(value));

    spi.tx_buf = (unsigned long)TXBuf;
    spi.rx_buf = (unsigned long)RXBuf;
    spi.len = sizeof(value);
    spi.speed_hz = speed;
    spi.delay_usecs = 0;
    spi.bits_per_word = 8;
    spi.cs_change = 0;

    err = ioctl(fd, SPI_IOC_MESSAGE(1), &spi);

    last_written_value = value; // Track last written value
}

void cleanup_bus(void)
{
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}