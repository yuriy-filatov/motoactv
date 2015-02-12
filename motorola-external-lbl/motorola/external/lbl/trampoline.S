/*
 * trampoline.S: start of second stage boot loader, jump from assembly to C
 *
 * Copyright (C) 2001 Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
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
 * 04-Oct-2006  Motorola        Removed BLOB specific includes and code.
 */

.ident "$Id: trampoline.S,v 1.3 2001/10/28 20:34:41 erikm Exp $"

#include "lbl.h"

.text

.globl _trampoline
_trampoline:

	@ When we get here, the registers are as follows.
	@ The registers are set in start.S.
	@
	@ r0:  ptr to BOOT_INFO structure
	@ r1:  second stage load address
	@
	@ We move the source registers to ones that are saved by the
	@ ARM C calling convention.
	@
	mov	r8, r0

	@ Load the address where we should relocate the Linux Boot
	@ Loader.  This code is already running at the load address.
	mov 	r0, r1

	@ Set up the stack pointer.
	@ Don't forget to relocate it.
	ldr	r2, stack_end
	add	r2, r2, r0		@ relocate the sp
	sub	sp, r2, #4

	@ Dynamically link the second stage.
	bl	dynlink
	cmp	r0, #0
        LBL_PANIC_IF(ne, LBL_PANIC_DYNLINK_FAILED, msg_dynlink_failed)

	@ Clear the BSS section.
	@ This code clears the stack.
	ldr	r1, bss_start
	ldr	r0, bss_end
	sub	r0, r0, r1
	mov	r2, #0

        @ At this point, the registers contain the following info.
	@ r1 = start address
	@ r0 = number of bytes
clear_bss:
	stmia	r1!, {r2}
	subs	r0, r0, #4
	bne	clear_bss

	@ Save the BOOT_INFO pointer and load address.
	ldr	r5, =boot_infop
	str	r8, [r5]

jump_to_main:
	@ jump to C code
	bl	main

	@ If main ever returns, we fail.
        LBL_PANIC(LBL_PANIC_MAIN_RETURNED, msg_main_returned)

#ifdef LBL_DEBUG
msg_dynlink_failed:
        .asciz  "Dynamic linking failed.\n"
msg_main_returned:
        .asciz  "LBL main() function returned.\n"
        .align
#endif /* LBL_DEBUG */

bss_start:	.word	__bss_start
bss_end:	.word	__bss_end
stack_end:	.word	__stack_end
