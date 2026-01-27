# M65832 Common Library

This directory contains shared ISA definitions and parsing code that is
synchronized from the main M65832 processor project.

## Source of Truth

The canonical versions of these files live in:
```
/Users/benjamincooley/projects/m65832/common/
```

**DO NOT edit the files in this directory directly.** Changes should be made
in the m65832 project and then synced here.

## Syncing

To sync changes from the m65832 project:

```bash
cd /Users/benjamincooley/projects/llvm-m65832
./scripts/sync-m65832-common.sh
```

Or set the `M65832_PROJECT` environment variable if your m65832 project is
in a different location:

```bash
M65832_PROJECT=/path/to/m65832 ./scripts/sync-m65832-common.sh
```

## Files

- `m65832_isa.h` - Instruction set definitions (addressing modes, opcode tables)
- `m65832_isa.c` - Opcode table implementations and lookup functions
- `m65832_parser.h` - Parser API for operands and instructions
- `m65832_parser.c` - Parser implementation

## Usage

These files are used by:
- LLVM AsmParser (`../AsmParser/`) - for inline assembly support
- Standalone assembler (`m65832as`) - shares same instruction definitions
- Standalone disassembler (`m65832dis`) - shares same opcode tables
