#!/bin/bash
# Build script for M65832 minimal libc

set -e

LLVM_BUILD=/Users/benjamincooley/projects/llvm-m65832/build
CC=$LLVM_BUILD/bin/clang-23
AR=ar
LD=$LLVM_BUILD/bin/lld
LLC=$LLVM_BUILD/bin/llc

PLATFORM_DIR=/Users/benjamincooley/projects/m65832/emu/platform

BUILD_DIR=build

CFLAGS="-target m65832 -O0 -ffreestanding -nostdlib -Ilibc/include -I$PLATFORM_DIR -Wall"

echo "=== Building M65832 minimal libc ==="
echo "CC: $CC"
echo "CFLAGS: $CFLAGS"
echo ""

mkdir -p $BUILD_DIR/{libc/{string,stdlib,stdio,ctype},platform,startup}

# Build libc string functions
echo "Building string functions..."
for f in libc/src/string/*.c; do
    obj=$BUILD_DIR/libc/string/$(basename $f .c).o
    echo "  Compiling $f"
    $CC $CFLAGS -c $f -o $obj
done

# Build libc stdlib functions
echo "Building stdlib functions..."
for f in libc/src/stdlib/*.c; do
    obj=$BUILD_DIR/libc/stdlib/$(basename $f .c).o
    echo "  Compiling $f"
    $CC $CFLAGS -c $f -o $obj
done

# Build libc stdio functions
echo "Building stdio functions..."
for f in libc/src/stdio/*.c; do
    obj=$BUILD_DIR/libc/stdio/$(basename $f .c).o
    echo "  Compiling $f"
    $CC $CFLAGS -c $f -o $obj
done

# Build libc ctype functions
echo "Building ctype functions..."
for f in libc/src/ctype/*.c; do
    obj=$BUILD_DIR/libc/ctype/$(basename $f .c).o
    echo "  Compiling $f"
    $CC $CFLAGS -c $f -o $obj
done

# Create libc.a
echo "Creating libc.a..."
$AR rcs $BUILD_DIR/libc.a $BUILD_DIR/libc/*/*.o

# Build platform functions
echo "Building platform functions..."
$CC $CFLAGS -c $PLATFORM_DIR/uart.c -o $BUILD_DIR/platform/uart.o
$CC $CFLAGS -c $PLATFORM_DIR/sys.c -o $BUILD_DIR/platform/sys.o

# Create libplatform.a
echo "Creating libplatform.a..."
$AR rcs $BUILD_DIR/libplatform.a $BUILD_DIR/platform/uart.o $BUILD_DIR/platform/sys.o

# Build startup code
echo "Building startup code..."
$CC -target m65832 -c startup/baremetal/crt0.s -o $BUILD_DIR/startup/crt0.o
$CC $CFLAGS -c startup/baremetal/init.c -o $BUILD_DIR/startup/init.o

cp $BUILD_DIR/startup/crt0.o $BUILD_DIR/crt0.o
cp $BUILD_DIR/startup/init.o $BUILD_DIR/init.o

echo ""
echo "=== Build complete ==="
echo "Libraries:"
ls -la $BUILD_DIR/*.a
echo ""
echo "Startup objects:"
ls -la $BUILD_DIR/*.o
