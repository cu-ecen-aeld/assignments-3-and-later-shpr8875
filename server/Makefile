# Makefile to compile aesdsocket.c natively
# Author: Shweta Prasad

# Use CROSS_COMPILE if set; otherwise default to gcc
ifdef CROSS_COMPILE
    ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
        CC ?= $(CROSS_COMPILE)gcc
    else
        CC ?= gcc
    endif
else
    CC ?= gcc
endif

# Target executable aesdsocket utility
TARGET := aesdsocket

# Source and object files
SRC := aesdsocket.c
OBJS := $(SRC:.c=.o)

# Compiler and linker flags
CFLAGS ?= -Werror -Wall
LDFLAGS ?= -lpthread -lrt

# All rule for compiling the target
all: $(TARGET)

# Rule for linking the target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS)

# Rule for compiling source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule for removing built files
clean:
	rm -f $(OBJS) $(TARGET)

