# M65832 Compiler Runtime Library
#
# Provides software implementations of 64-bit arithmetic
# and floating-point conversion functions.

LLVM_BUILD ?= /Users/benjamincooley/projects/llvm-m65832/build-fast
CC = $(LLVM_BUILD)/bin/clang
AR = ar

SYSROOT ?= /Users/benjamincooley/projects/m65832-sysroot

CFLAGS = -target m65832-elf -O0 -ffreestanding -fno-builtin
CFLAGS += -Wall -Wextra

SRCS = compiler_rt.c compiler_rt_sret.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean install

all: libcompiler_rt.a

libcompiler_rt.a: $(OBJS)
	$(AR) rcs $@ $^
	@echo "Built: $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: libcompiler_rt.a
	cp libcompiler_rt.a $(SYSROOT)/lib/
	@echo "Installed to $(SYSROOT)/lib/"

clean:
	rm -f $(OBJS) libcompiler_rt.a
