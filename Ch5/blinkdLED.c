// blinkLED.c
// Sends a blink‐count command to the FPGA and then fetches
// the up‐to‐date blink count after every half‐second blink.

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SPI_SPEED_HZ 1000000    // 1 MHz
#define HALF_SECOND_US 500000   // 0.5 s


// Open and configure the SPI device
int spiOpen(unsigned spiChan, unsigned spiBaud, unsigned spiFlags) {
  int i, fd;
  char spiMode;
  char spiBits = 8;
  char dev[32];

  spiMode = spiFlags & 3;
  spiBits = 8;

  sprintf(dev, "/dev/spidev0.%d", spiChan);

  if ((fd = open(dev, O_RDWR)) < 0) {
    return -1;
  }

  if (ioctl(fd, SPI_IOC_WR_MODE, &spiMode) < 0) {
    close(fd);
    return -2;
  }

  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spiBits) < 0) {
    close(fd);
    return -3;
  }

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spiBaud) < 0) {
    close(fd);
    return -4;
  }

  return fd;
}

int spiClose(int fd) { return close(fd); }

// Write count to the FPGA
int spiWrite(int fd, unsigned speed, char *buf, unsigned count) {
  int err;
  struct spi_ioc_transfer spi;

  memset(&spi, 0, sizeof(spi));

  spi.tx_buf = (unsigned)buf;
  spi.rx_buf = (unsigned)NULL;
  spi.len = count;
  spi.speed_hz = speed;
  spi.delay_usecs = 0;
  spi.bits_per_word = 8;
  spi.cs_change = 0;

  err = ioctl(fd, SPI_IOC_MESSAGE(1), &spi);

  return err;
}

// Read count from the FPGA
int spiRead(int fd, unsigned speed, char *buf, unsigned count) {
  int err;
  struct spi_ioc_transfer spi;

  memset(&spi, 0, sizeof(spi));

  spi.tx_buf = (unsigned)NULL;
  spi.rx_buf = (unsigned)buf;
  spi.len = count;
  spi.speed_hz = speed;
  spi.delay_usecs = 0;
  spi.bits_per_word = 8;
  spi.cs_change = 0;

  err = ioctl(fd, SPI_IOC_MESSAGE(1), &spi);

  return err;
}

int main(int argc, char *argv[]) {
    int spi_fd;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_blinks>\n", argv[0]);
        return 1;
    }
    int target = atoi(argv[1]);
    if (target < 1 || target > 255) {
        fprintf(stderr, "Error: blink count must be 1–255\n");
        return 1;
    }

    // Open SPI
    spi_fd = spiOpen(1, SPI_SPEED_HZ, 0);
    if (spi_fd < 0) {
        fprintf(stderr, "spiOpen failed: %d\n", spi_fd);
        return 1;
    }

    // Send the command byte (how many blinks)
    {
        char tx = (char)target;
        if (spiWrite(spi_fd, SPI_SPEED_HZ, &tx, 1) < 0) {
            perror("spiWrite");
            spiClose(spi_fd);
            return 1;
        }
    }

    // After each 0.5 s blink, read back the current count
    for (int i = 1; i <= target; i++) {
        usleep(HALF_SECOND_US); // wait 500 ms
        
        char rx = 0;
        if (spiRead(spi_fd, SPI_SPEED_HZ, &rx, 1) < 0) {
            perror("spiRead");
            break;
        }

        printf("Blink %2d/%2d  —  FPGA reports: %u\n", i, target, (uint8_t)rx);
        fflush(stdout);
    }

    // Close SPI
    spiClose(spi_fd);
    return 0;
}
