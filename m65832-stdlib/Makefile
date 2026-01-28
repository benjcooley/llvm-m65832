# Makefile for M65832 Standard Library
#
# Builds:
#   - libc.a: Minimal C library
#   - libplatform.a: Platform hardware layer (emulator-specific)
#   - crt0.o: C runtime startup
#
# Usage:
#   make                    # Build everything
#   make clean              # Clean build artifacts
#   make test               # Build test program

# Toolchain
LLVM_BUILD ?= /Users/benjamincooley/projects/llvm-m65832/build-fast
CC = $(LLVM_BUILD)/bin/clang
AS = $(LLVM_BUILD)/bin/clang
AR = ar
LD = $(LLVM_BUILD)/bin/ld.lld

# Platform directory (emulator hardware layer - no LLVM dependency)
PLATFORM_DIR ?= /Users/benjamincooley/projects/m65832/emu/platform

# Flags
CFLAGS = -target m65832 -O2 -ffreestanding -nostdlib
CFLAGS += -Ilibc/include -I$(PLATFORM_DIR)
CFLAGS += -Wall -Wextra

ASFLAGS = -target m65832

# Output
BUILD_DIR = build

# Libc sources
STRING_SRC = $(wildcard libc/src/string/*.c)
STDLIB_SRC = $(wildcard libc/src/stdlib/*.c)
STDIO_SRC = $(wildcard libc/src/stdio/*.c)
CTYPE_SRC = $(wildcard libc/src/ctype/*.c)

LIBC_SRC = $(STRING_SRC) $(STDLIB_SRC) $(STDIO_SRC) $(CTYPE_SRC)
LIBC_OBJ = $(patsubst libc/src/%.c,$(BUILD_DIR)/libc/%.o,$(LIBC_SRC))

# Platform sources (from emulator - no LLVM dependency)
PLATFORM_SRC = $(PLATFORM_DIR)/uart.c $(PLATFORM_DIR)/sys.c
PLATFORM_OBJ = $(patsubst $(PLATFORM_DIR)/%.c,$(BUILD_DIR)/platform/%.o,$(PLATFORM_SRC))

# Startup sources
STARTUP_C_SRC = $(wildcard startup/baremetal/*.c)
STARTUP_S_SRC = $(wildcard startup/baremetal/*.s)
STARTUP_C_OBJ = $(patsubst startup/baremetal/%.c,$(BUILD_DIR)/startup/%.o,$(STARTUP_C_SRC))
STARTUP_S_OBJ = $(patsubst startup/baremetal/%.s,$(BUILD_DIR)/startup/%.o,$(STARTUP_S_SRC))

.PHONY: all clean test info

all: $(BUILD_DIR)/libc.a $(BUILD_DIR)/libplatform.a $(BUILD_DIR)/crt0.o $(BUILD_DIR)/init.o

# Libc archive
$(BUILD_DIR)/libc.a: $(LIBC_OBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	@echo "Built: $@"

# Platform archive
$(BUILD_DIR)/libplatform.a: $(PLATFORM_OBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	@echo "Built: $@"

# CRT0 object (from C startup - assembly version needs standalone assembler)
$(BUILD_DIR)/crt0.o: $(BUILD_DIR)/startup/crt0.o
	@mkdir -p $(dir $@)
	cp $< $@
	@echo "Built: $@"

# Build crt0.o from C source explicitly (not .s, which needs standalone assembler)
$(BUILD_DIR)/startup/crt0.o: startup/baremetal/crt0.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Init object (for global constructors/destructors)
$(BUILD_DIR)/init.o: $(BUILD_DIR)/startup/init.o
	@mkdir -p $(dir $@)
	cp $< $@
	@echo "Built: $@"

# Compile libc sources
$(BUILD_DIR)/libc/%.o: libc/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile platform sources
$(BUILD_DIR)/platform/%.o: $(PLATFORM_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile startup C files
$(BUILD_DIR)/startup/%.o: startup/baremetal/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup assembly files
$(BUILD_DIR)/startup/%.o: startup/baremetal/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Test program
test: all $(BUILD_DIR)/test.elf
	@echo "Built test program: $(BUILD_DIR)/test.elf"

$(BUILD_DIR)/test.elf: $(BUILD_DIR)/test.o $(BUILD_DIR)/crt0.o $(BUILD_DIR)/init.o \
                       $(BUILD_DIR)/libc.a $(BUILD_DIR)/libplatform.a
	$(LD) -T scripts/baremetal/m65832.ld -o $@ \
		$(BUILD_DIR)/crt0.o $(BUILD_DIR)/init.o $(BUILD_DIR)/test.o \
		$(BUILD_DIR)/libc.a $(BUILD_DIR)/libplatform.a

$(BUILD_DIR)/test.o: test/hello.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

info:
	@echo "LIBC_SRC: $(LIBC_SRC)"
	@echo "PLATFORM_SRC: $(PLATFORM_SRC)"
	@echo "STARTUP_C_SRC: $(STARTUP_C_SRC)"
	@echo "STARTUP_S_SRC: $(STARTUP_S_SRC)"
