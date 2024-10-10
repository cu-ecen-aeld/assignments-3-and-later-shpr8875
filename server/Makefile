# Makefile to cross-compile aesdsocket.c
# Author: Shweta Prasad

# Ensure CROSS_COMPILE is set, otherwise throw an error
ifndef CROSS_COMPILE
    $(error CROSS_COMPILE is not set)
endif

# Set the compiler using CROSS_COMPILE variable
CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Werror -g

# Target executable
TARGET := aesdsocket

# Source and target for compilation
all: $(TARGET)

# Build the target using the cross-compiler
$(TARGET): aesdsocket.c
$(CC) $(CFLAGS) aesdsocket.c -o $(TARGET)

# Clean up previous builds
clean:
rm -f $(TARGET)
