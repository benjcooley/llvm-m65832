#!/bin/bash
# build_newlib.sh - Clone and build newlib for M65832
#
# This script:
# 1. Clones newlib to an adjacent folder if not present
# 2. Checks out a specific release tag for reproducibility
# 3. Builds newlib with the M65832 cross-compiler
# 4. Installs to a local prefix for use by the toolchain

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STDLIB_DIR="$(dirname "$SCRIPT_DIR")"
PROJECTS_DIR="$(dirname "$(dirname "$STDLIB_DIR")")"

# Newlib version - use a specific tag for reproducible builds
NEWLIB_VERSION="newlib-4.4.0"

# Directories
NEWLIB_SRC="$PROJECTS_DIR/newlib"
NEWLIB_BUILD="$PROJECTS_DIR/newlib-build-m65832"
INSTALL_PREFIX="$PROJECTS_DIR/m65832-sysroot"
LLVM_BUILD="$PROJECTS_DIR/llvm-m65832/build"

# Tools
CLANG="$LLVM_BUILD/bin/clang"
LLVM_AR="$LLVM_BUILD/bin/llvm-ar"
LLVM_RANLIB="$LLVM_BUILD/bin/llvm-ranlib"

echo "=========================================="
echo "M65832 Newlib Build Script"
echo "=========================================="
echo ""
echo "Newlib:    $NEWLIB_VERSION"
echo "Source:    $NEWLIB_SRC"
echo "Build:     $NEWLIB_BUILD"
echo "Install:   $INSTALL_PREFIX"
echo "LLVM:      $LLVM_BUILD"
echo ""

# Check for LLVM tools
if [ ! -x "$CLANG" ]; then
    echo "ERROR: clang not found at $CLANG"
    echo "Please build LLVM first: cd llvm-m65832/build && cmake --build . --target clang"
    exit 1
fi

# Step 1: Clone newlib if needed
if [ ! -d "$NEWLIB_SRC" ]; then
    echo ">>> Cloning newlib (tag: $NEWLIB_VERSION)..."
    git clone --branch "$NEWLIB_VERSION" --depth 1 \
        https://sourceware.org/git/newlib-cygwin.git "$NEWLIB_SRC"
    echo ">>> Newlib $NEWLIB_VERSION cloned successfully"
else
    echo ">>> Newlib source exists at $NEWLIB_SRC"
    # Verify we're on the correct version
    cd "$NEWLIB_SRC"
    CURRENT_TAG=$(git describe --tags --exact-match 2>/dev/null || echo "unknown")
    if [ "$CURRENT_TAG" != "$NEWLIB_VERSION" ]; then
        echo "    WARNING: Current version is $CURRENT_TAG, expected $NEWLIB_VERSION"
        echo "    To update, remove $NEWLIB_SRC and re-run this script"
    else
        echo "    Version: $NEWLIB_VERSION"
    fi
fi

# Step 2: Create build directory
mkdir -p "$NEWLIB_BUILD"
mkdir -p "$INSTALL_PREFIX"

# Step 3: Configure newlib
echo ""
echo ">>> Configuring newlib for m65832-elf..."
cd "$NEWLIB_BUILD"

# Check if already configured
if [ ! -f "Makefile" ]; then
    "$NEWLIB_SRC/configure" \
        --target=m65832-elf \
        --prefix="$INSTALL_PREFIX" \
        --disable-newlib-supplied-syscalls \
        --enable-newlib-reent-small \
        --disable-newlib-fvwrite-in-abs \
        --disable-newlib-fseek-optimization \
        --disable-newlib-wide-orient \
        --enable-newlib-nano-malloc \
        --disable-newlib-unbuf-stream-opt \
        --enable-lite-exit \
        --enable-newlib-global-atexit \
        --enable-newlib-nano-formatted-io \
        --disable-multilib \
        CC_FOR_TARGET="$CLANG -target m65832-elf -ffreestanding" \
        AS_FOR_TARGET="$CLANG -target m65832-elf -ffreestanding" \
        AR_FOR_TARGET="$LLVM_AR" \
        RANLIB_FOR_TARGET="$LLVM_RANLIB" \
        CFLAGS_FOR_TARGET="-O2 -g"
    echo ">>> Configuration complete"
else
    echo ">>> Already configured"
fi

# Step 4: Build
echo ""
echo ">>> Building newlib..."
make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

# Step 5: Install
echo ""
echo ">>> Installing to $INSTALL_PREFIX..."
make install

# Step 6: Copy our syscalls and crt0
echo ""
echo ">>> Installing M65832-specific files..."
M65832_LIB="$INSTALL_PREFIX/m65832-elf/lib"
mkdir -p "$M65832_LIB"

# Compile syscalls
"$CLANG" -target m65832-elf -ffreestanding -O2 -c \
    "$STDLIB_DIR/newlib/syscalls.c" -o "$M65832_LIB/syscalls.o"

# Compile crt0
"$CLANG" -target m65832-elf -ffreestanding -O2 -c \
    "$STDLIB_DIR/newlib/crt0.c" -o "$M65832_LIB/crt0.o"

# Copy linker script
cp "$STDLIB_DIR/newlib/m65832.ld" "$M65832_LIB/"

# Create combined startup object
"$LLVM_AR" rcs "$M65832_LIB/libsys.a" "$M65832_LIB/syscalls.o"

echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
echo ""
echo "Installed to: $INSTALL_PREFIX"
echo ""
echo "To use newlib in your projects:"
echo ""
echo "  # Compile"
echo "  clang -target m65832-elf -ffreestanding \\"
echo "    -I$INSTALL_PREFIX/m65832-elf/include \\"
echo "    -c myprogram.c -o myprogram.o"
echo ""
echo "  # Link"
echo "  ld.lld -T $M65832_LIB/m65832.ld \\"
echo "    $M65832_LIB/crt0.o myprogram.o \\"
echo "    -L$M65832_LIB -lc -lsys -lm \\"
echo "    -o myprogram.elf"
echo ""
