; crt0.s - M65832 C Runtime Startup for Picolibc (Assembly version)
;
; Entry point for C programs. Sets up:
; 1. Stack pointer  
; 2. D register for direct page addressing (0x4000)
; 3. BSS initialization (zero fill)
; 4. Data section copy (if load != start)
; 5. Calls __libc_init_array, main, __libc_fini_array, _exit
;
; This is written in assembly to avoid compiler crashes with crt0.c

    .text
    .globl _start
    .type _start, @function

_start:
    ; Set up stack pointer
    ldx #_stack_top
    txs
    
    ; CRITICAL: Initialize D register for direct page addressing
    ; D = 0x4000 is where GPRs R0-R63 are mapped in memory
    ; Using explicit bytes to avoid assembler encoding issues
    .byte 0xA9, 0x00, 0x40, 0x00, 0x00   ; LDA #$00004000
    .byte 0x5B                            ; TCD
    
    ; Initialize B register to 0 for absolute JSR B+addr calls
    ; SB #0 - Set B from immediate (6 bytes: $02 $22 + 32-bit value)
    .byte 0x02, 0x22, 0x00, 0x00, 0x00, 0x00  ; SB #$00000000
    
    ; Fall through to __crt_init
    
    .globl __crt_init  
    .type __crt_init, @function
__crt_init:
    ;
    ; Initialize BSS to zero
    ; R0 = current pointer, R1 = end pointer
    ;
    
    ; R0 = _bss_start
    .byte 0xA9                            ; LDA #imm32
    .long _bss_start                      ; (linker will fill in address)
    .byte 0x85, 0x00                      ; STA dp $00 (R0)
    
    ; R1 = _bss_end
    .byte 0xA9                            ; LDA #imm32
    .long _bss_end
    .byte 0x85, 0x04                      ; STA dp $04 (R1)
    
    ; Compare R0 < R1
    .byte 0xA5, 0x00                      ; LDA dp $00 (R0)
    .byte 0xC5, 0x04                      ; CMP dp $04 (R1)
    bcs .Lbss_done                        ; if R0 >= R1, skip
    
.Lbss_loop:
    ; Store zero at address in R0
    .byte 0xA9, 0x00, 0x00, 0x00, 0x00    ; LDA #0
    ldy #0
    .byte 0x91, 0x00                      ; STA (R0),Y
    
    ; R0 += 4
    clc
    .byte 0xA5, 0x00                      ; LDA dp $00 (R0)
    .byte 0x69, 0x04, 0x00, 0x00, 0x00    ; ADC #4
    .byte 0x85, 0x00                      ; STA dp $00 (R0)
    
    ; Loop while R0 < R1
    .byte 0xC5, 0x04                      ; CMP dp $04 (R1)
    bcc .Lbss_loop
    
.Lbss_done:

    ;
    ; Copy initialized data from _data_load to _data_start (if different)
    ; R0 = dst (data_start), R1 = end (data_end), R2 = src (data_load)
    ;
    
    ; Check if copy needed: _data_load != _data_start
    .byte 0xA9                            ; LDA #imm32
    .long _data_load
    .byte 0x85, 0x08                      ; STA dp $08 (R2 = src)
    
    .byte 0xA9                            ; LDA #imm32
    .long _data_start
    .byte 0x85, 0x00                      ; STA dp $00 (R0 = dst)
    
    ; Compare: if src == dst, skip copy
    .byte 0xA5, 0x08                      ; LDA dp $08 (src)
    .byte 0xC5, 0x00                      ; CMP dp $00 (dst)
    beq .Ldata_done
    
    ; R1 = _data_end
    .byte 0xA9                            ; LDA #imm32
    .long _data_end
    .byte 0x85, 0x04                      ; STA dp $04 (R1)
    
.Ldata_loop:
    ; Check if done: dst >= end
    .byte 0xA5, 0x00                      ; LDA dp $00 (dst)
    .byte 0xC5, 0x04                      ; CMP dp $04 (end)
    bcs .Ldata_done
    
    ; Copy word: *dst = *src
    ldy #0
    .byte 0xB1, 0x08                      ; LDA (R2),Y  - load from src
    .byte 0x91, 0x00                      ; STA (R0),Y  - store to dst
    
    ; dst += 4
    clc
    .byte 0xA5, 0x00                      ; LDA dp $00
    .byte 0x69, 0x04, 0x00, 0x00, 0x00    ; ADC #4
    .byte 0x85, 0x00                      ; STA dp $00
    
    ; src += 4
    clc
    .byte 0xA5, 0x08                      ; LDA dp $08
    .byte 0x69, 0x04, 0x00, 0x00, 0x00    ; ADC #4
    .byte 0x85, 0x08                      ; STA dp $08
    
    bra .Ldata_loop
    
.Ldata_done:

    ;
    ; Call C library initialization (constructors)
    ; In 32-bit mode, JSR is 5 bytes: opcode 0x20 + 32-bit address
    ; Address is added to B (which is 0) to get absolute target
    ;
    .byte 0x20                            ; JSR opcode
    .long __libc_init_array               ; 32-bit absolute address
    
    ;
    ; Call main()
    ;
    .byte 0x20                            ; JSR opcode
    .long main                            ; 32-bit absolute address
    
    ;
    ; Get return value from R0 (ABI: return value is in R0)
    ;
    .byte 0xA5, 0x00                      ; LDA dp $00 (load R0)
    pha                                   ; Save return value
    
    ;
    ; Call C library finalization (destructors)
    ;
    .byte 0x20                            ; JSR opcode
    .long __libc_fini_array               ; 32-bit absolute address
    
    ;
    ; Exit with return value
    ;
    pla                                   ; Restore return value
    .byte 0x85, 0x00                      ; STA dp $00 (put in R0 for _exit)
    .byte 0x20                            ; JSR opcode
    .long _exit                           ; 32-bit absolute address
    
    ; Should never reach here
    stp

.Lfunc_end:
    .size _start, .Lfunc_end - _start
    .size __crt_init, .Lfunc_end - __crt_init

;
; Weak default symbols (linker overrides if picolibc provides them)
;
    .weak __libc_init_array
    .type __libc_init_array, @function
__libc_init_array:
    rts
    .size __libc_init_array, . - __libc_init_array

    .weak __libc_fini_array
    .type __libc_fini_array, @function  
__libc_fini_array:
    rts
    .size __libc_fini_array, . - __libc_fini_array

    .weak _exit
    .type _exit, @function
_exit:
    stp
    .size _exit, . - _exit
