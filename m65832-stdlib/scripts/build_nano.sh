#!/bin/bash
# build_nano.sh - Build the minimal "nano" libc for M65832
#
# This is Stage 1 of the libc bootstrap:
# - String functions (memcpy, strcpy, strcmp, strlen, etc.)
# - Memory functions (memset, memmove)
# - Ctype functions (isalpha, isdigit, toupper, etc.)
# - Basic stdio (puts, putchar, printf basics)
# - Stdlib basics (abs, atoi, malloc/free)
#
# Once this works, we can move to full picolibc (Stage 2).

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STDLIB_DIR="$(dirname "$SCRIPT_DIR")"
PROJECTS_DIR="$(dirname "$(dirname "$STDLIB_DIR")")"

# Build directories
BUILD_DIR="$STDLIB_DIR/build"
INSTALL_PREFIX="$PROJECTS_DIR/m65832-nano-sysroot"

# Tools
LLVM_BUILD="$PROJECTS_DIR/llvm-m65832/build"
CLANG="$LLVM_BUILD/bin/clang"
# Use system ar/ranlib since llvm-ar may not be built
AR="ar"
RANLIB="ranlib"

# Source directories
LIBC_SRC="$STDLIB_DIR/libc/src"
LIBC_INC="$STDLIB_DIR/libc/include"
PLATFORM_DIR="$PROJECTS_DIR/M65832/emu/platform"
STARTUP_DIR="$STDLIB_DIR/startup/baremetal"

# Compiler flags
CFLAGS="-target m65832-elf -O2 -ffreestanding -nostdlib -Wall -Wextra"
CFLAGS="$CFLAGS -I$LIBC_INC -I$PLATFORM_DIR"

echo "=========================================="
echo "M65832 Nano Libc Build"
echo "=========================================="
echo ""
echo "Source:    $LIBC_SRC"
echo "Build:     $BUILD_DIR"
echo "Install:   $INSTALL_PREFIX"
echo ""

# Check for clang
if [ ! -x "$CLANG" ]; then
    echo "ERROR: clang not found at $CLANG"
    exit 1
fi

# Create directories
mkdir -p "$BUILD_DIR"/{libc,platform,startup}
mkdir -p "$INSTALL_PREFIX"/{lib,include}

# Function to compile a source file
compile() {
    local src="$1"
    local obj="$2"
    echo "  CC $src"
    "$CLANG" $CFLAGS -c "$src" -o "$obj"
}

# Step 1: Build libc
echo ">>> Building nano libc..."

# String functions
for src in "$LIBC_SRC"/string/*.c; do
    name=$(basename "$src" .c)
    compile "$src" "$BUILD_DIR/libc/${name}.o"
done

# Ctype functions
compile "$LIBC_SRC/ctype/ctype.c" "$BUILD_DIR/libc/ctype.o"

# Stdio functions
compile "$LIBC_SRC/stdio/stdio.c" "$BUILD_DIR/libc/stdio.o"

# Stdlib functions
compile "$LIBC_SRC/stdlib/stdlib.c" "$BUILD_DIR/libc/stdlib.o"

# Create libc.a
echo ""
echo ">>> Creating libc.a..."
"$AR" rcs "$BUILD_DIR/libc.a" "$BUILD_DIR"/libc/*.o
"$RANLIB" "$BUILD_DIR/libc.a"

# Step 2: Build platform support
echo ""
echo ">>> Building platform support..."
compile "$PLATFORM_DIR/uart.c" "$BUILD_DIR/platform/uart.o"
compile "$PLATFORM_DIR/sys.c" "$BUILD_DIR/platform/sys.o"

"$AR" rcs "$BUILD_DIR/libplatform.a" "$BUILD_DIR"/platform/*.o
"$RANLIB" "$BUILD_DIR/libplatform.a"

# Step 3: Build startup code
echo ""
echo ">>> Building startup code..."
compile "$STARTUP_DIR/crt0.c" "$BUILD_DIR/startup/crt0.o"
compile "$STARTUP_DIR/init.c" "$BUILD_DIR/startup/init.o"

# Step 4: Install
echo ""
echo ">>> Installing to $INSTALL_PREFIX..."

# Libraries
cp "$BUILD_DIR/libc.a" "$INSTALL_PREFIX/lib/"
cp "$BUILD_DIR/libplatform.a" "$INSTALL_PREFIX/lib/"
cp "$BUILD_DIR/startup/crt0.o" "$INSTALL_PREFIX/lib/"
cp "$BUILD_DIR/startup/init.o" "$INSTALL_PREFIX/lib/"

# Headers
cp -r "$LIBC_INC"/* "$INSTALL_PREFIX/include/"

# Linker script
cp "$STDLIB_DIR/scripts/baremetal/m65832.ld" "$INSTALL_PREFIX/lib/"

echo ""
echo "=========================================="
echo "Nano Libc Build Complete!"
echo "=========================================="
echo ""
echo "Installed to: $INSTALL_PREFIX"
echo ""
echo "Libraries:"
ls -la "$INSTALL_PREFIX/lib/"
echo ""
echo "To use:"
echo "  clang -target m65832-elf -ffreestanding -nostdlib \\"
echo "    -I$INSTALL_PREFIX/include \\"
echo "    -c myprogram.c -o myprogram.o"
echo ""
echo "  ld.lld -T $INSTALL_PREFIX/lib/m65832.ld \\"
echo "    $INSTALL_PREFIX/lib/crt0.o $INSTALL_PREFIX/lib/init.o \\"
echo "    myprogram.o -L$INSTALL_PREFIX/lib -lc -lplatform \\"
echo "    -o myprogram.elf"
echo ""
