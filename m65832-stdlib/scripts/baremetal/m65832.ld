/* m65832.ld - M65832 Linker Script
 *
 * Memory Map:
 *   $0000-$00FF  - Direct Page (Zero Page) - used for fast access
 *   $0100-$01FF  - Hardware Stack (6502-style, used by JSR/RTS)
 *   $0200-$0FFF  - Reserved for I/O or special use
 *   $1000-$7FFF  - ROM/Code region (.text)
 *   $8000-$BFFF  - RAM/Data region (.data, .rodata)
 *   $C000-$EFFF  - BSS region (.bss)
 *   $F000-$FEFF  - Heap region
 *   $FF00-$FFFF  - Software stack (grows down from $FFFF)
 *
 * For ROM-based systems, .data is copied from ROM to RAM at startup.
 * For RAM-only systems (emulator), everything can be in RAM.
 */

/* OUTPUT_FORMAT for M65832 - let lld auto-detect */
ENTRY(_start)

/* Define program headers - no PT_PHDR to avoid loading headers at addr 0 */
PHDRS
{
  text PT_LOAD FLAGS(5);   /* R-X */
  data PT_LOAD FLAGS(6);   /* RW- */
  bss  PT_LOAD FLAGS(6);   /* RW- */
}

MEMORY
{
  ZEROPAGE (rw)  : ORIGIN = 0x0000, LENGTH = 0x0100
  HWSTACK  (rw)  : ORIGIN = 0x0100, LENGTH = 0x0100
  IO       (rw)  : ORIGIN = 0x0200, LENGTH = 0x0E00
  CODE     (rx)  : ORIGIN = 0x1000, LENGTH = 0x7000
  DATA     (rw)  : ORIGIN = 0x8000, LENGTH = 0x4000
  BSS      (rw)  : ORIGIN = 0xC000, LENGTH = 0x3000
  HEAP     (rw)  : ORIGIN = 0xF000, LENGTH = 0x0F00
  STACK    (rw)  : ORIGIN = 0xFF00, LENGTH = 0x0100
}

SECTIONS
{
  /* Code section - starts at $1000 */
  .text : {
    *(.text._start)     /* Entry point first */
    *(.text.startup)    /* Startup code */
    *(.text)            /* All other code */
    *(.text.*)
    . = ALIGN(4);
  } > CODE :text

  /* Read-only data - placed after code */
  .rodata : {
    *(.rodata)
    *(.rodata.*)
    . = ALIGN(4);
  } > CODE :text

  /* Initialized data - in DATA region */
  .data : {
    __data_start = .;
    *(.data)
    *(.data.*)
    . = ALIGN(4);
    __data_end = .;
  } > DATA :data

  /* Uninitialized data - in BSS region */
  .bss : {
    __bss_start = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > BSS :bss

  /* Stack - at top of memory, grows downward */
  .stack (NOLOAD) : {
    . = ALIGN(4);
    __stack_bottom = .;
    . = . + 0x100;
    __stack_top = .;
  } > STACK :NONE

  /* Heap - between BSS and stack */
  .heap (NOLOAD) : {
    __heap_start = .;
    . = ALIGN(4);
    __heap_end = ORIGIN(HEAP) + LENGTH(HEAP);
  } > HEAP :NONE

  /* Provide symbols for runtime */
  PROVIDE(__bss_start = __bss_start);
  PROVIDE(__bss_end = __bss_end);
  PROVIDE(__data_start = __data_start);
  PROVIDE(__data_end = __data_end);
  PROVIDE(__stack_top = __stack_top);
  PROVIDE(__heap_start = __heap_start);
  PROVIDE(__heap_end = __heap_end);

  /* Discard unwanted sections */
  /DISCARD/ : {
    *(.comment)
    *(.note.*)
    *(.eh_frame)
    *(.eh_frame_hdr)
  }
}
