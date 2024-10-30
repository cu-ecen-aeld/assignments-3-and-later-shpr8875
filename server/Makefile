# Makefile to compile aesdsocket.c natively
# Author: Shweta Prasad

# Set the compiler
CC := gcc
CFLAGS := -Wall -Werror -g

# Target executable
TARGET := aesdsocket

# Source and target for compilation
all: $(TARGET)

# Build the target using the native compiler
$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) aesdsocket.c -o $(TARGET)

# Clean up previous builds
clean:
	rm -f $(TARGET)
