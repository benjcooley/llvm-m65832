#!/bin/bash
# run-m65832.sh - Execute M65832 ELF binary on emulator
# Used by Meson as exe_wrapper for running picolibc tests
#
# Usage: run-m65832.sh <program.elf> [args...]
#
# Returns: Exit code from program (stored at memory location 0xFFFFFFFC)

EMU="/Users/benjamincooley/projects/M65832/emu/m65832emu"
MAX_CYCLES=1000000

if [ ! -x "$EMU" ]; then
    echo "Error: Emulator not found at $EMU" >&2
    exit 127
fi

PROG="$1"
shift

if [ ! -f "$PROG" ]; then
    echo "Error: Program not found: $PROG" >&2
    exit 127
fi

# Run on emulator with cycle limit
# The emulator loads ELF, runs until STP instruction, and reports exit code
OUTPUT=$("$EMU" -c "$MAX_CYCLES" "$PROG" 2>&1)
EMU_EXIT=$?

# Check for emulator errors
if [ $EMU_EXIT -ne 0 ]; then
    echo "Emulator error (exit $EMU_EXIT):" >&2
    echo "$OUTPUT" >&2
    exit $EMU_EXIT
fi

# Extract exit code from emulator output
# The emulator reports: "Exit code: N" or we check A register at STP
EXIT_CODE=$(echo "$OUTPUT" | grep -i "exit code:" | sed 's/.*exit code: *\([0-9]*\).*/\1/i')

if [ -z "$EXIT_CODE" ]; then
    # Try to get A register value as exit code
    EXIT_CODE=$(echo "$OUTPUT" | grep "A:" | tail -1 | sed 's/.*A: *\([0-9A-Fa-f]*\).*/\1/')
    if [ -n "$EXIT_CODE" ]; then
        # Convert hex to decimal
        EXIT_CODE=$((16#$EXIT_CODE))
    else
        EXIT_CODE=0
    fi
fi

# Output program output (if any)
echo "$OUTPUT" | grep -v "^PC:" | grep -v "^Exit code:" | grep -v "^Cycles:"

exit $EXIT_CODE
