# Makefile to compile aesdsocket.c natively
# Author: Shweta Prasad

# Default to gcc if CROSS_COMPILE is not set
CC := $(CROSS_COMPILE)gcc

# Compiler flags
CFLAGS := -Wall -Werror -g

# Target executable
TARGET := aesdsocket

# Source file
SRC := aesdsocket.c

# All rule for compiling the target
all: $(TARGET)

# Rule for building the target using the chosen compiler
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Clean rule for removing built files
clean:
	rm -f $(TARGET)
