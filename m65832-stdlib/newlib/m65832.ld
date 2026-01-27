/* m65832.ld - Linker script for M65832 baremetal with newlib */

OUTPUT_FORMAT("elf32-m65832")
OUTPUT_ARCH(m65832)
ENTRY(_start)

/* Memory layout for baremetal M65832 */
MEMORY
{
    /* ROM/Flash for code - starts at 0x1000 (after zero page and stack) */
    ROM (rx)  : ORIGIN = 0x1000, LENGTH = 48K
    
    /* RAM for data - starts after ROM */
    RAM (rwx) : ORIGIN = 0xC000, LENGTH = 12K
    
    /* Direct page (zero page) - first 256 bytes for fast access */
    DP  (rw)  : ORIGIN = 0x0000, LENGTH = 256
}

/* Stack grows down from 0xFFFF */
_stack_top = 0xFFFF;

SECTIONS
{
    /* Code section */
    .text :
    {
        . = ALIGN(4);
        _text_start = .;
        
        /* Startup code first */
        *(.text.startup)
        *(.text.startup.*)
        
        /* Then all other code */
        *(.text)
        *(.text.*)
        
        /* Read-only data can go in ROM too */
        *(.rodata)
        *(.rodata.*)
        
        . = ALIGN(4);
        _text_end = .;
    } > ROM

    /* C++ constructors */
    .init_array :
    {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(.init_array*))
        KEEP(*(SORT(.init_array.*)))
        __init_array_end = .;
    } > ROM

    /* C++ destructors */
    .fini_array :
    {
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP(*(.fini_array*))
        KEEP(*(SORT(.fini_array.*)))
        __fini_array_end = .;
    } > ROM

    /* Initialized data - loaded from ROM, copied to RAM */
    .data :
    {
        . = ALIGN(4);
        _data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _data_end = .;
    } > RAM AT > ROM
    
    _data_load = LOADADDR(.data);

    /* Uninitialized data (BSS) */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    /* Heap - everything after BSS until end of RAM */
    .heap (NOLOAD) :
    {
        . = ALIGN(4);
        _end = .;           /* Used by sbrk */
        _heap_start = .;
        . = ORIGIN(RAM) + LENGTH(RAM) - 256;  /* Leave 256 bytes for stack */
        _heap_end = .;
    } > RAM

    /* Direct page variables (fast access) */
    .dp (NOLOAD) :
    {
        *(.dp)
        *(.dp.*)
    } > DP

    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame*)
    }
}

/* Provide symbols for C library */
PROVIDE(_end = _bss_end);
PROVIDE(end = _bss_end);
