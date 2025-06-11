# Target selection
TARGET ?= spi
BUS ?= $(TARGET)

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -IPositionControllerPan -IPositionControllerTilt
LDFLAGS = -lpthread

# GStreamer flags
GSTREAMER_CFLAGS = $(shell pkg-config --cflags gstreamer-1.0 gstreamer-app-1.0 glib-2.0)
GSTREAMER_LIBS = $(shell pkg-config --libs gstreamer-1.0 gstreamer-app-1.0 glib-2.0)

# Add GStreamer flags to compiler and linker
CFLAGS += $(GSTREAMER_CFLAGS)
LDFLAGS += $(GSTREAMER_LIBS)

# Source files
COMMON_SOURCES = src/motor_control.c src/image_processing.c PositionControllerPan/xxmodelPan.c PositionControllerTilt/xxmodelTilt.c

# Bus implementation selection (use spaces, not tabs)
ifeq ($(BUS), spi)
    TARGET_SOURCES = src/spi_bus.c
else ifeq ($(BUS), avalon)
    TARGET_SOURCES = src/avalon_bus.c
else ifeq ($(BUS), sim)
    TARGET_SOURCES = src/sim_bus.c
else
    $(error Unknown bus type: $(BUS). Use spi, avalon, or sim)
endif

# All sources
SOURCES = $(COMMON_SOURCES) $(TARGET_SOURCES)
OBJECTS = $(SOURCES:.c=.o)

# Output
TARGET_NAME = motor_control_$(BUS)

# Build rules
all: $(TARGET_NAME)

$(TARGET_NAME): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) motor_control_*


spi:
	$(MAKE) BUS=spi

avalon:
	$(MAKE) BUS=avalon

sim:
	$(MAKE) BUS=sim

.PHONY: all clean spi avalon sim
