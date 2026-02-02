#!/bin/bash
# run_tests.sh - Configure and run picolibc test suite for M65832
#
# This uses the picolibc-m65832 fork with a Meson cross file and the
# M65832 emulator wrapper. It keeps the test build dir separate from the
# install-only build.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STDLIB_DIR="$(dirname "$SCRIPT_DIR")"
PROJECTS_DIR="$(dirname "$(dirname "$STDLIB_DIR")")"

PICOLIBC_SRC="$PROJECTS_DIR/picolibc-m65832"
PICOLIBC_BUILD="$PROJECTS_DIR/picolibc-build-m65832-tests"
INSTALL_PREFIX="$PROJECTS_DIR/m65832-sysroot"
LLVM_BUILD="$PROJECTS_DIR/llvm-m65832/build-fast"

CROSS_FILE="$STDLIB_DIR/picolibc/cross-m65832.txt"

echo "=========================================="
echo "M65832 Picolibc Test Suite"
echo "=========================================="
echo ""
echo "Source:    $PICOLIBC_SRC"
echo "Build:     $PICOLIBC_BUILD"
echo "Install:   $INSTALL_PREFIX"
echo "LLVM:      $LLVM_BUILD"
echo ""

if [ ! -d "$PICOLIBC_SRC" ]; then
  echo "ERROR: picolibc source not found at $PICOLIBC_SRC"
  echo "Run: $STDLIB_DIR/scripts/build_picolibc.sh"
  exit 1
fi

if ! command -v meson >/dev/null 2>&1; then
  echo "ERROR: meson not found. Install with: pip install meson"
  exit 1
fi

show_usage() {
  echo "Usage: $0 [meson-test-args...]"
  echo ""
  echo "Examples:"
  echo "  $0 -v"
  echo "  $0 --list"
  echo "  $0 string"
}

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
  show_usage
  exit 0
fi

PRINTF_ALIASES="${PICOLIBC_PRINTF_ALIASES:-false}"

# Configure
meson setup "$PICOLIBC_BUILD" "$PICOLIBC_SRC" \
  --cross-file "$CROSS_FILE" \
  --prefix="$INSTALL_PREFIX" \
  --buildtype=plain \
  -Ddebug=false \
  -Doptimization=1 \
  -Dmultilib=false \
  -Dtests=true \
  -Dprintf-aliases="$PRINTF_ALIASES" \
  -Dspecsdir=none \
  -Dfreestanding=true

# Build + test
meson compile -C "$PICOLIBC_BUILD"
meson test -C "$PICOLIBC_BUILD" -v "$@"
