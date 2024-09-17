# Makefile to run writer.c
# Author: Shweta Prasad

CC := gcc

# Setting up cross-complier
ifdef CROSS_COMPILE
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS := -Wall -Werror -g 

# Source and target for compliation
SRC := writer.c
TARGET := writer

all: $(TARGET)

# For building writer.c
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(CFLAGS)

# Clean up previous build
clean:
	rm -f $(TARGET)
