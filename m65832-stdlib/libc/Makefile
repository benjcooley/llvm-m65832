# Minimal libc for M65832 baremetal
#
# This builds libc.a which can be linked with applications.
# The platform layer (uart.c, sys.c) must be provided separately.

CC = clang
AR = llvm-ar

CFLAGS = -target m65832 -ffreestanding -nostdlib -O2 \
         -I$(CURDIR)/include \
         -I$(PLATFORM_DIR)

# Platform directory (emulator platform layer)
PLATFORM_DIR ?= /Users/benjamincooley/projects/m65832/emu/platform

# Source files
STRING_SRCS = $(wildcard src/string/*.c)
STDLIB_SRCS = $(wildcard src/stdlib/*.c)
STDIO_SRCS = $(wildcard src/stdio/*.c)
CTYPE_SRCS = $(wildcard src/ctype/*.c)

LIBC_SRCS = $(STRING_SRCS) $(STDLIB_SRCS) $(STDIO_SRCS) $(CTYPE_SRCS)
LIBC_OBJS = $(LIBC_SRCS:.c=.o)

# Platform sources (compiled separately but can be included)
PLATFORM_SRCS = $(PLATFORM_DIR)/uart.c $(PLATFORM_DIR)/sys.c
PLATFORM_OBJS = $(notdir $(PLATFORM_SRCS:.c=.o))

# Output
LIBC = libc.a
LIBPLATFORM = libplatform.a

.PHONY: all clean

all: $(LIBC) $(LIBPLATFORM)

$(LIBC): $(LIBC_OBJS)
	$(AR) rcs $@ $^

$(LIBPLATFORM): $(PLATFORM_OBJS)
	$(AR) rcs $@ $^

# Compile libc sources
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile platform sources
uart.o: $(PLATFORM_DIR)/uart.c
	$(CC) $(CFLAGS) -c $< -o $@

sys.o: $(PLATFORM_DIR)/sys.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBC_OBJS) $(PLATFORM_OBJS) $(LIBC) $(LIBPLATFORM)

# For debugging
print-srcs:
	@echo "LIBC_SRCS: $(LIBC_SRCS)"
	@echo "LIBC_OBJS: $(LIBC_OBJS)"
