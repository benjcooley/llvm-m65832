#!/bin/bash
# run-m65832.sh - Wrapper script to run M65832 ELF binaries on the emulator
#
# This is used by meson as the exe_wrapper for running tests.
# It runs the given ELF file on the m65832 emulator and returns the exit code.

# Find the emulator
EMU="${M65832_EMU:-/Users/benjamincooley/projects/M65832/emu/m65832emu}"

if [ ! -x "$EMU" ]; then
    echo "ERROR: M65832 emulator not found at $EMU" >&2
    echo "Set M65832_EMU environment variable to emulator path" >&2
    exit 77  # Skip test (meson convention)
fi

# Get the ELF file (first argument)
ELF="$1"
shift

if [ ! -f "$ELF" ]; then
    echo "ERROR: ELF file not found: $ELF" >&2
    exit 1
fi

# Run on emulator with reasonable instruction limit
# Pass remaining arguments via semihosting if supported
exec "$EMU" "$ELF" -n 1000000 "$@"
