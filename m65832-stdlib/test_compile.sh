#!/bin/bash
# Quick test to verify M65832 compiler works with libc

CC=/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang
PLATFORM=/Users/benjamincooley/projects/m65832/emu/platform

echo "Testing M65832 compiler with libc..."
echo "Compiler: $CC"

# Test compile strlen.c
echo ""
echo "Compiling strlen.c..."
$CC -target m65832 -O2 -ffreestanding -nostdlib \
    -Ilibc/include -I$PLATFORM \
    -c libc/src/string/strlen.c -o /tmp/strlen.o

if [ $? -eq 0 ]; then
    echo "SUCCESS: strlen.c compiled"
    ls -la /tmp/strlen.o
else
    echo "FAILED: strlen.c compilation failed"
    exit 1
fi

# Test compile printf.c
echo ""
echo "Compiling stdio.c..."
$CC -target m65832 -O2 -ffreestanding -nostdlib \
    -Ilibc/include -I$PLATFORM \
    -c libc/src/stdio/stdio.c -o /tmp/stdio.o

if [ $? -eq 0 ]; then
    echo "SUCCESS: stdio.c compiled"
    ls -la /tmp/stdio.o
else
    echo "FAILED: stdio.c compilation failed"
    exit 1
fi

echo ""
echo "All tests passed!"
