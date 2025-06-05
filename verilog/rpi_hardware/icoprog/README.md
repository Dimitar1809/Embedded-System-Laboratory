# IcoProg -- a modern IcoBoard uploader

Tool to upload IcoBoard bitstreams to the IcoBoard shield.
Using the modern libgpiod library to access the Raspberry Pi's GPIO.

# Compilation

```bash
apt install libgpiod-dev
g++ src/icoprog.cpp src/gpio_interface.cpp -o icoprog -lgpiodcxx
```
