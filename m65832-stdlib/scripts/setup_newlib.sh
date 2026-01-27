#!/bin/bash
# Setup newlib for M65832 target
# This script clones newlib to an adjacent folder and prepares it for building

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STDLIB_DIR="$(dirname "$SCRIPT_DIR")"
PROJECTS_DIR="$(dirname "$(dirname "$STDLIB_DIR")")"
NEWLIB_DIR="$PROJECTS_DIR/newlib"
LLVM_BUILD="$PROJECTS_DIR/llvm-m65832/build"

echo "=========================================="
echo "M65832 Newlib Setup"
echo "=========================================="
echo ""
echo "Projects dir: $PROJECTS_DIR"
echo "Newlib dir:   $NEWLIB_DIR"
echo "LLVM build:   $LLVM_BUILD"
echo ""

# Check for required tools
if [ ! -x "$LLVM_BUILD/bin/clang-23" ]; then
    echo "ERROR: clang-23 not found at $LLVM_BUILD/bin/clang-23"
    echo "Please build LLVM first."
    exit 1
fi

# Clone newlib if not present
if [ ! -d "$NEWLIB_DIR" ]; then
    echo "Cloning newlib..."
    git clone --depth 1 https://sourceware.org/git/newlib-cygwin.git "$NEWLIB_DIR"
else
    echo "Newlib already exists at $NEWLIB_DIR"
fi

# Create m65832 system directory in newlib
M65832_SYS="$NEWLIB_DIR/newlib/libc/sys/m65832"
if [ ! -d "$M65832_SYS" ]; then
    echo "Creating m65832 system directory..."
    mkdir -p "$M65832_SYS"
    
    # Copy our syscall implementations
    cp "$STDLIB_DIR/newlib/syscalls.c" "$M65832_SYS/"
    cp "$STDLIB_DIR/newlib/crt0.S" "$M65832_SYS/"
    cp "$STDLIB_DIR/newlib/Makefile.am" "$M65832_SYS/"
    cp "$STDLIB_DIR/newlib/configure.in" "$M65832_SYS/"
    
    echo "M65832 system files installed"
else
    echo "M65832 system directory already exists"
fi

echo ""
echo "Setup complete!"
echo ""
echo "To build newlib for m65832:"
echo "  cd $NEWLIB_DIR"
echo "  mkdir build-m65832 && cd build-m65832"
echo "  ../configure --target=m65832-elf \\"
echo "    CC_FOR_TARGET=$LLVM_BUILD/bin/clang-23 \\"
echo "    AR_FOR_TARGET=$LLVM_BUILD/bin/llvm-ar \\"
echo "    AS_FOR_TARGET=$LLVM_BUILD/bin/clang-23 \\"
echo "    RANLIB_FOR_TARGET=$LLVM_BUILD/bin/llvm-ranlib"
echo "  make"
