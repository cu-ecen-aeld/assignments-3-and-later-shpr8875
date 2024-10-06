# Makefile to run aesdsocket.c
# Author: Shweta Prasad

# aarch64-none-linux-gnu-gcc or gcc

CC = gcc
ifdef CROSS_COMPILE
#aarch64-none-linux-gnu-
CC = $(CROSS_COMPILE)gcc
endif
CFLAGS := -Wall -Werror -g 

# Target executable
TARGET := aesdsocket

# Source and target for compliation
all: $(TARGET)

# For building writer.c
$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) aesdsocket.c -o $(TARGET)

# Clean up previous build
clean:
	rm -f $(TARGET)
