/*
 * Copyright (c) 2010 Innofidei Inc.
 * 
 */
#include <arm.h>
	
	.section ".start", #alloc, #execinstr
	.type _start, #function
	.global _start

_start:
	ldr r0,=0xe1200108
	ldr r1,[r0]
	add r1,r1,#(4<<2)
	mov pc,r1
	@ set to SVC mode and disable all interrupts
	msr 	cpsr_c, #(PSR_I_BIT | PSR_F_BIT | SVC_MODE)

	@ disable MMU if enabled
	/*
	mrc		p15, 0, r0, c1, c0, 0	@ read CP15 control register
	tst		r0, #0x1				@ is MMU enabled ?
	bicne	r0, r0, #0x1			@ clear bit0
	mcrne	p15, 0, r0, c1, c0, 0	@ write value back
	*/

	 MRC     p15, 0, r0, c1, c0, 0           //; Get control register
        BIC     r0, r0, #(1 << 0)          //   ; Disable MMU
        BIC     r0, r0, #(1 << 2)          //   ; Disable D Cache
        BIC     r0, r0, #(1 << 12)            // ; Disable I Cache
        MCR     p15, 0, r0, c1, c0, 0         //  ; Write control register
        MRC     p15, 0, r0, c1, c0, 1          // ; Get Auxiliary control register
        BIC     r0, r0, #(1 << 1)     //     ; Disable L2 Cache
        MCR     p15, 0, r0, c1, c0, 1         //  ; Write Auxiliary control register
	
	@ are we running from our VMA
	ldr		r0, =1f
	adr		r1, 1f
	cmp		r1, r0
	bxeq	r0

	@ copy ourselves to our VMA
relocate:
	adr		r1, _start
	ldr		r2, =__text_start
	ldr		r3, =__bss_start

copy_loop:
	ldmia	r1!, {r4-r11}
	stmia	r2!, {r4-r11}
	cmp		r2, r3
	blt		copy_loop

	@ ensure we are running the code at the address it linked at
	bx		r0

	@ clear bss section
1:	ldr		r1, bss_start	@ r1 = start address
	ldr		r0, bss_end
	subs	r0, r0, r1		@ r0 = number of bytes
	beq		setup_stack
	mov		r2, #0
	
clear_bss:
	stmia	r1!, {r2}
	subs	r0, r0, #4
	bne		clear_bss

	@ stack setup
setup_stack:
	ldr		r0, stack_end
	sub		sp, r0, #4

	@jump code main
  	ldr r0, =main
	blx r0


bss_start:	.word	__bss_start
bss_end:	.word	__bss_end
stack_end:	.word	__stack_end

	.section ".stack", "aw", %nobits
user_stack:	.space	8192
user_stack_end:
