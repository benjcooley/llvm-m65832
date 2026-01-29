/* m65832.ld - Linker script for M65832 baremetal with picolibc */
/* Memory layout for 1MB address space */
/*
 * All code and data use full 32-bit addressing.
 * JSR/RTS use 32-bit absolute addresses in 32-bit mode.
 * JSL/RTL are reserved (illegal) in 32-bit mode.
 */

/* Note: OUTPUT_FORMAT/OUTPUT_ARCH not needed - lld infers from input files */
ENTRY(_start)

/* Memory layout for baremetal M65832 (1MB) */
MEMORY
{
    /* ROM/Flash for code - starts at 128K to ensure 32-bit addressing */
    ROM (rx)  : ORIGIN = 0x00020000, LENGTH = 384K
    
    /* RAM for data - at 512K */
    RAM (rwx) : ORIGIN = 0x00080000, LENGTH = 512K
    
    /* Direct page area - GPRs are mapped at D+0x00 through D+0xFF
     * D register is set to 0x4000 in crt0, so this is 0x4000-0x40FF
     * We don't allocate anything here - it's for register access */
    DP  (rw)  : ORIGIN = 0x00004000, LENGTH = 256
}

/* Stack grows down from top of 1MB - MUST be 4-byte aligned for ABI */
_stack_top = 0x000FFFFC;

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
    /* Note: For ELF loaders that load to vaddr, we don't use AT > ROM
     * because the loader already places data at the correct address.
     * For ROM-based systems, change to: > RAM AT > ROM */
    .tdata :
    {
        . = ALIGN(4);
        __tdata_start = .;
        *(.tdata .tdata.*)
        __tdata_end = .;
    } > RAM
    
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

    /* Initialized data section
     * Note: For ELF loaders that load to vaddr, we don't use AT > ROM
     * because the loader already places data at the correct address.
     * For ROM-based systems, change to: > RAM AT > ROM */
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
    } > RAM
    
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

    /* Heap - everything after BSS until near top of RAM (leave room for stack) */
    .heap (NOLOAD) :
    {
        . = ALIGN(4);
        _end = .;
        end = .;
        _heap_start = .;
        /* Leave 64KB for stack at top of RAM */
        . = ORIGIN(RAM) + LENGTH(RAM) - 64K;
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
