/*
 * puts-asm.S: LBL debugging support. Output an ASCIIZ string using putchar.
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

#include "debug.h"

.text

/* ********************************************************************
 * _puts
 *
 * Send an ASCIIZ string to the UART.
 *
 * Parameters:
 *   r0 - base address of the UART to which to send character
 *   r1 - address of the string to send
 * Return Value:
 *   None.
 *
 * Side Effects:
 *   r1, r2, r3, r4, r5, r6, and r7 are destroyed.
 *
 * Internal Register Usage:
 *   ro - base address of UART
 *   r1 - next byte to send to UART
 *   r2 - destroyed by _putchar
 *   r3 - destroyed by _putchar
 *   r4 - destroyed by _putchar
 *   r5 - address of the next byte
 *   r6 - characters left before reaching send limit
 *   r7 - preserved link register
 *
 * ********************************************************************/
.globl _puts
_puts:
        /* preserve the link register */
        mov     r7, lr

        /* load the send limit */
        ldr     r6, =(PUTS_SEND_LIMIT)

        /* move the string address to r5 */
        mov     r5, r1

_send_str_loop:
        /* decrement send limit */
        subs    r6, r6, #1

        /* if send limit is zero, abort */
        beq     _send_str_done

        /* load the next byte of the string */
        ldrb    r1, [r5]

        /* test to see if the byte is 0 */
        orrs    r1, r1, #0x00
        beq     _send_str_done

        /* send the byte */
        bl      _putchar

        /* move to next address and start over */
        add     r5, r5, #0x1
        b       _send_str_loop

_send_str_done:
        /* return from this function */
        mov     pc, r7
