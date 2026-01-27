#!/bin/bash
#
# Sync M65832 common library from the processor project to LLVM.
#
# Source of truth: /Users/benjamincooley/projects/m65832/as/common/
# Destination:     llvm/lib/Target/M65832/Common/
#
# Run this script from the llvm-m65832 directory after making changes
# to the instruction set or parser in the m65832 project.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LLVM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

M65832_PROJECT="${M65832_PROJECT:-/Users/benjamincooley/projects/m65832}"
M65832_COMMON="$M65832_PROJECT/as/common"
LLVM_COMMON="$LLVM_ROOT/llvm/lib/Target/M65832/Common"

# Files to sync
FILES=(
    "m65832_isa.h"
    "m65832_isa.c"
    "m65832_parser.h"
    "m65832_parser.c"
)

echo "Syncing M65832 common library..."
echo "  Source: $M65832_COMMON"
echo "  Dest:   $LLVM_COMMON"
echo ""

# Check source directory exists
if [ ! -d "$M65832_COMMON" ]; then
    echo "ERROR: Source directory not found: $M65832_COMMON"
    echo "Set M65832_PROJECT environment variable to the m65832 project root."
    exit 1
fi

# Create destination if needed
mkdir -p "$LLVM_COMMON"

# Sync each file
for file in "${FILES[@]}"; do
    src="$M65832_COMMON/$file"
    dst="$LLVM_COMMON/$file"
    
    if [ ! -f "$src" ]; then
        echo "WARNING: Source file not found: $src"
        continue
    fi
    
    if [ -f "$dst" ]; then
        if diff -q "$src" "$dst" > /dev/null 2>&1; then
            echo "  [unchanged] $file"
        else
            cp "$src" "$dst"
            echo "  [updated]   $file"
        fi
    else
        cp "$src" "$dst"
        echo "  [new]       $file"
    fi
done

echo ""
echo "Sync complete."
echo ""
echo "Don't forget to rebuild LLVM if instruction definitions changed:"
echo "  cd build && ninja"
