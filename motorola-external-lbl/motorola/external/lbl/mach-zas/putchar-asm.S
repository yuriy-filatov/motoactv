/*
 * putchar-asm.S: ZAS debugging output driver. Uses UART to report status.
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
 * _putchar
 *
 * Send a single character to the UART.
 *
 * Parameters:
 *   r0 - base address of the UART to which to send character
 *   r1 - character to send to serial port
 * Return Value:
 *   None.
 *
 * Side Effects:
 *   r2, r3, and r4 are destroyed.
 *
 * Internal Register Usage:
 *   ro - base address of UART
 *   r1 - next byte to send to UART
 *   r2 - value loaded from UTS
 *   r3 - preserved link register in _putchar_cr_lf
 *   r4 - retry count
 *
 * ********************************************************************/
.globl _putchar
_putchar:
        /* load the retry count */
        ldr     r4, =(PUTCHAR_RETRIES)

        /* check to see if output is a newline */
        cmp     r1, #NEWLINE
        beq     _putchar_cr_lf

_putchar_raw:
        /* decrement retry count */
        subs    r4, r4, #1

        /* give up if retry count is zero */
        beq     _putchar_return

        /* check to see if there is room in the transmit buffer */
        ldrh    r2, [r0, #UTS]
        ands    r2, r2, #UTS_TXFULL /* mask out all bits but TXFULL */
        bne     _putchar_raw  /* wait if TXFULL is set */

        /* send the data */
        strh    r1, [r0, #UTXD]

_putchar_return:
        /* return from this function */
        mov     pc, lr

_putchar_cr_lf:
        /* preserve the link register */
        mov     r3, lr

        /* send carriage return */
        mov     r1, #CARRIAGERET
        bl      _putchar_raw

        /* reload the retry count */
        ldr     r4, =(PUTCHAR_RETRIES)

        /* send line feed */
        mov     r1, #NEWLINE
        bl      _putchar_raw

        /* return from this function */
        mov     pc, r3
