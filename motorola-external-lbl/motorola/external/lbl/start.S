/*
 * start.S: lbl start code
 *
 * Copyright 2006 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Initial revision.
 */

#include "lbl.h"
#include "boot_info.h"
#include "geninc.h"

#ifdef LBL_DEBUG
#include "debug.h"
#endif /* LBL_DEBUG */

.text

.globl _start

_start:
	b	real_start

	@
	@ The lbl_barker must remain at the second word so that other
	@ code can find it.
	@
lbl_barker:	.word	LBL_BARKER
lbl_version:    .asciz  LBL_VERSION_STR(LBL_VERSION)

#ifdef LBL_DEBUG
msg_bad_boot_info_ptr:
        .asciz "Bad boot information pointer.\n"
msg_no_boot_info_barker:
        .asciz "Missing or invalid boot information block marker.\n"
msg_bad_boot_info_version:
        .asciz "Incompatible boot information block version.\n"
#endif /* LBL_DEBUG */
	.align

	@ This is the real start to the lbl code.
real_start:
	@ Save the pointer to the BOOT_INFO structure.
	mov	r12, r0

#ifdef LBL_DEBUG
        @ _init_debug destroys r1
	ldr	r0, =STDOUT_DEVICE
	bl      _init_debug

        @ _puts destroys r1, r2, r3, r4, and r5
        adr     r1, lbl_version
        bl      _puts

        @ send a new line; _putchar destroys r2 and r3
        mov     r1, #NEWLINE
        bl      _putchar
#endif /* LBL_DEBUG */

	@ Validate the BOOT_INFO pointer.
	ands	r1, r12, #3			@ must be word aligned
        LBL_PANIC_IF(ne, LBL_PANIC_BAD_BOOT_INFO_PTR, msg_bad_boot_info_ptr)

	ldr	r2, [r12, #BOOT_INFO_BARKER_OFF] @ provided BOOT_INFO barker
	ldr	r3, =BOOT_INFO_BARKER		@ expected BOOT_INFO barker
	cmp	r2, r3
        LBL_PANIC_IF(ne, LBL_PANIC_NO_BOOT_INFO_BARKER, msg_no_boot_info_barker)

boot_info_version_chk:
	ldr	r2, [r12, #BOOT_INFO_VERSION_OFF] @ provided BOOT_INFO version
	ldr	r3, =BOOT_INFO_MINVERSION	 @ min version supported
	cmp	r2, r3
        LBL_PANIC_IF(lt, LBL_PANIC_BAD_BOOT_INFO_VERSION,
            msg_bad_boot_info_version)

#if 0
	@ We don't reinitialize the processor mode or the MMU,
	@ but leave the code for future debugging.
	@ Note: Reusing the MMU setup from the MBM provides
	@       exception handling during the LBL execution.

	@ set CPU to SVC32 mode
	mrs	r0, cpsr
	bic	r0, r0, #0x1f
	orr	r0, r0, #0x13
	msr	cpsr, r0

	mcr     p15, 0, r0, c7, c10, 4  @ drain WB
	mcr     p15, 0, r0, c7, c7, 0   @ flush I & D caches

	@ disabling MMU and caches
	mrc     p15, 0, r0, c1, c0, 0   @ read control reg
	bic     r0, r0, #0x05           @ clear DC, MMU
	bic     r0, r0, #0x1000         @ clear Icache
	mcr     p15, 0, r0, c1, c0, 0
	mcr     p15, 0, r1, c7, c5, 0   @ invalidate icache and BTB
	mov	r1, r1
#endif

chk_relocate:
	@ If the src and dest are equal, then we're already running
	@ at the load address.  We skip the relocation.
	adr	r0, _start		@ src addr
	ldr	r1, [r12, #BOOT_INFO_OS_LDR_LOAD_ADDR_OFF] @ dest addr
	cmp	r0, r1
	beq     no_relocate		@ skip relocation (already done)

relocate:
	@ Check the src/dest ranges for overlap.
	@ We check for overlap of these ranges:
	@ r0: src
	@ r1: dest
	@ r2: src+<1st stage size>+<2nd stage size>
	@ r3: dest+<2nd stage size>
	ldr	r2, =LBL_REST_SIZE	@ lbl_rest size (bytes)
	mov	r3, r2			@ dest size (bytes)
	ldr	r8, =__bss_end		@ size of 1st stage
	add	r2, r2, r8		@ add in size of 1st stage

	@ check_overlap promises to only modify r4 and leave
	@ everything else unchanged.
	bl	check_overlap

	@ Set up for copy.
	add	r0, r0, r8		@ skip first stage
	mov	r2, r3			@ r3 contains lbl_rest size
	bic	r3, r2, #0x1f		@ multiple of 32 bytes
	add	r2, r0, r3		@ r2 = source end address

	@ Copy the second stage loader to its load addr.
	@ The registers at this point contain these addrs.
	@ r0 = source address
	@ r1 = target address
	@ r2 = source end address
	@
	@ The second stage is copied in two steps.  The first step
	@ copies 32 byte blocks and the second step copies any
	@ remaining bytes, which are known to be a multiple of 4
	@ bytes, so we copy whole words.
	@ 
block_copy_loop:
	ldmia	r0!, {r4-r11}
	stmia	r1!, {r4-r11}
	cmp	r0, r2
	blt	block_copy_loop

	ldr	r2, =LBL_REST_SIZE	@ lbl_rest size (bytes)
	sub	r2, r2, r3		@ remaining bytes to copy
	add	r2, r0, r2
byte_copy_loop:
	ldmia	r0!, {r3}
	stmia	r1!, {r3}
	cmp	r0, r2
	blt	byte_copy_loop

	@ Load the second stage load address.
	ldr	r1, [r12, #BOOT_INFO_OS_LDR_LOAD_ADDR_OFF]

	b	prep_for_jump


no_relocate:
	@ If we get here, then the Linux Boot Loader is already
	@ executing at its load address.  The second stage needs
	@ to have the second stage load address so that it can
	@ dynamically link itself.  We add the requested load address
	@ from the BOOT_INFO structure and the size of the first stage
	@ to get the second stage load address.
	ldr	r8, =__bss_end		@ size of 1st stage
	add	r1, r1, r8		@ second stage load addr

prep_for_jump:
	@ Set up registers that the second stage expects.
	@ r0:  address of BOOT_INFO struct
	@ r1:  load address of second stage
	@      r1 is already set up
	@
	mov	r0, r12

jump_to_ram:
	mov	pc, r1

@
@ check_overlap checks whether the two given ranges overlap.
@ r0:  src
@ r1:  dest
@ r2:  src size in bytes
@ r3:  dest size in bytes
@ 
@ check_overlap uses only registers r0-r4 and leaves r0-r3
@ unchanged if it actually returns.
@
check_overlap:
        cmp     r0, r1			@ if (src>=dest) ...
        bcs     next_check		@     goto next_check
        add     r4, r0, r2		@ we know src<dest
        cmp     r4, r1			@ if ((src+srcsize)<dest) ...
        bls     no_overlap		@     there's no overlap
next_check:
        cmp     r1, r0			@ if (src<dest) ...
        @ they're equal so overlap
        LBL_PANIC_IF(cs, LBL_PANIC_RELOC_OVERLAP, msg_reloc_overlap)

        add     r4, r1, r3		@ we know dest<src
        cmp     r4, r0			@ if ((dest+destsize)>src) ...
        @ overlap detected
        LBL_PANIC_IF(hi, LBL_PANIC_RELOC_OVERLAP, msg_reloc_overlap)

no_overlap:
	mov	pc, lr

#ifdef LBL_DEBUG
msg_reloc_overlap:
        .asciz "Relocation addresses overlap.\n"
        .align
#endif /* LBL_DEBUG */

@ **********************************************************************
@ * lbl_fail
@ *
@ * Indicate that the Linux Boot Loader has failed to boot linux.
@ *
@ * Parameters:
@ *     r0 - numeric code indicating failure type
@ *     r1 - address of ASCIIZ string with error message
@ * Return Value:
@ *     This function never returns. r0 holds the error code.
@ *
@ * Note: This function's use of registers are arranged in such a way to
@ * be compatible with the C language version of lbl_fail.
@ **********************************************************************
lbl_fail:
#ifdef LBL_DEBUG
        @ preserve the failure code
        mov     r8, r0

        @ display the error message on the UART
        @ _puts destroys r1, r2, r3, r4, and r5
        ldr     r0, =STDOUT_DEVICE
        bl      _puts

        @ final destination of error code is r0
        mov     r0, r8
#endif /* LBL_DEBUG */

infinite_loop:
        b	infinite_loop		@ infinite loop
