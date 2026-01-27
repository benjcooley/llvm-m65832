; crt0.s - M65832 Baremetal C Runtime Startup
;
; Entry point for bare metal C programs. This file:
; 1. Sets up the stack pointer
; 2. Initializes BSS to zero
; 3. Calls global constructors
; 4. Calls main()
; 5. Calls global destructors
; 6. Halts with return value

	.text
	.globl	_start
	.type	_start,@function

_start:
	; Set up hardware stack (for JSR/RTS)
	LDX	#$01FF
	TXS
	
	; Set up software stack pointer (frame pointer R29)
	LD.L	R29,#__stack_top
	
	; Initialize BSS section to zero
	JSR	B+__init_bss
	
	; Call global constructors (__init_array)
	JSR	B+__libc_init_array
	
	; Call main(argc=0, argv=NULL)
	LD.L	R0,#$00
	LD.L	R1,#$00
	JSR	B+main
	
	; Save return value
	PHA
	
	; Call global destructors (__fini_array)
	JSR	B+__libc_fini_array
	
	; Restore return value to A
	PLA
	
	; Halt processor
	STP

.Lfunc_end_start:
	.size	_start, .Lfunc_end_start-_start


; Initialize BSS section to zero
	.globl	__init_bss
	.type	__init_bss,@function
__init_bss:
	LD.L	R0,#__bss_start
	LD.L	R1,#__bss_end
	CMPR	R0,R1
	BCS	.bss_done
	LD.L	R2,#$00
.bss_loop:
	LDY	#$00
	LDA	R2
	STA	(R0),Y
	INC	R0
	CMPR	R0,R1
	BCC	.bss_loop
.bss_done:
	RTS
.Lfunc_end_init_bss:
	.size	__init_bss, .Lfunc_end_init_bss-__init_bss


; sys_exit - halt processor with exit code in A
	.globl	sys_exit
	.type	sys_exit,@function
sys_exit:
	; Exit code already in R0, move to A for convention
	LDA	R0
	STP
.Lfunc_end_sys_exit:
	.size	sys_exit, .Lfunc_end_sys_exit-sys_exit


; sys_abort - halt processor immediately (abnormal termination)
	.globl	sys_abort
	.type	sys_abort,@function
sys_abort:
	; No cleanup, just halt
	STP
.Lfunc_end_sys_abort:
	.size	sys_abort, .Lfunc_end_sys_abort-sys_abort


; sys_sbrk - simple heap allocation (bump pointer)
	.globl	sys_sbrk
	.type	sys_sbrk,@function
sys_sbrk:
	; R0 = increment amount
	; Returns old heap pointer in R0, or -1 on failure
	LD.L	R1,#__heap_ptr
	LDA	(R1)
	STA	R2			; R2 = old pointer (return value)
	CLC
	ADC	R0			; A = new pointer
	; Check if we've exceeded heap end
	CMP.L	#__heap_end
	BCS	.sbrk_fail
	STA	(R1)			; Store new heap pointer
	LDA	R2			; Return old pointer
	STA	R0
	RTS
.sbrk_fail:
	LD.L	R0,#$FFFFFFFF		; Return -1
	RTS
.Lfunc_end_sys_sbrk:
	.size	sys_sbrk, .Lfunc_end_sys_sbrk-sys_sbrk


; Heap pointer (initialized to __heap_start by linker)
	.data
	.globl	__heap_ptr
__heap_ptr:
	.long	__heap_start


; Weak default symbols (overridden by linker)
	.weak	__bss_start
	.weak	__bss_end
	.weak	__stack_top
	.weak	__heap_start
	.weak	__heap_end
	.weak	__init_array_start
	.weak	__init_array_end
	.weak	__fini_array_start
	.weak	__fini_array_end
	.weak	main
