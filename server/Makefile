# Makefile to run writer.c
# Author: Shweta Prasad

CC := gcc
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
