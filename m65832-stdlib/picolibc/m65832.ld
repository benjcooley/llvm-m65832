/* m65832.ld - Linker script for M65832 baremetal with picolibc */

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
        KEEP(*(.text.startup))
        KEEP(*(.text.startup.*))
        
        /* Then all other code */
        *(.text)
        *(.text.*)
        
        /* Read-only data can go in ROM too */
        *(.rodata)
        *(.rodata.*)
        *(.srodata)
        *(.srodata.*)
        
        . = ALIGN(4);
        _text_end = .;
    } > ROM

    /* C++ constructors */
    .init_array :
    {
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array*))
        __preinit_array_end = .;
        
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    } > ROM

    /* C++ destructors */
    .fini_array :
    {
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
    } > ROM

    /* TLS template (thread-local storage) */
    .tdata :
    {
        . = ALIGN(4);
        __tdata_start = .;
        *(.tdata .tdata.*)
        __tdata_end = .;
    } > RAM AT > ROM
    
    __tdata_source = LOADADDR(.tdata);
    __tdata_size = __tdata_end - __tdata_start;

    .tbss (NOLOAD) :
    {
        . = ALIGN(4);
        __tbss_start = .;
        *(.tbss .tbss.*)
        __tbss_end = .;
    } > RAM
    
    __tbss_size = __tbss_end - __tbss_start;
    __tls_size = __tbss_end - __tdata_start;
    __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss));

    /* Initialized data - loaded from ROM, copied to RAM */
    .data :
    {
        . = ALIGN(4);
        _data_start = .;
        *(.data)
        *(.data.*)
        *(.sdata)
        *(.sdata.*)
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
        *(.sbss)
        *(.sbss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    /* Heap - everything after BSS until end of RAM */
    .heap (NOLOAD) :
    {
        . = ALIGN(4);
        _end = .;
        end = .;
        _heap_start = .;
        . = ORIGIN(RAM) + LENGTH(RAM) - 256;
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
        *(.ARM.*)
    }
}

/* Provide symbols for C library */
PROVIDE(_end = _bss_end);
PROVIDE(end = _bss_end);
PROVIDE(__heap_start = _heap_start);
PROVIDE(__heap_end = _heap_end);
