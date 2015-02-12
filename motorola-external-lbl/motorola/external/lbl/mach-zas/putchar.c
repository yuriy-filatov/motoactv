/*
 * putchar.c: C stdio-compatible putchar routine for LBL debugging output.
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

#ifdef LBL_DEBUG
#include "debug.h"
#endif /* LBL_DEBUG */
#include "../util.h"

/* *********************************************************************
 * putchar_raw
 *
 * Write a single character to the output device. If LBL_DEBUG is defined,
 * the output device is the UART mapped into memory address at specified by
 * STDOUT_DEVICE; otherwise, this function does nothing.
 *
 * For Zeus/Argon/SCM-A11, debugging output device is UART 3.
 *
 * Parameters:
 *   c - character to be written. Is cast to unsigned char.
 *
 * Return Value:
 *   Character written to output device or -1 on error.
 *
 * ********************************************************************/
static int putchar_raw(int c)
{
    /* LBL_DEBUG must be defined for the UART initialization code to be
     * compiled into lbl_start. */
#ifdef LBL_DEBUG
    volatile unsigned short int *uts_reg =
        (unsigned short int *)(STDOUT_DEVICE + UTS);

    volatile unsigned short int *utxd_reg =
        (unsigned short int *)(STDOUT_DEVICE + UTXD);

    /* number of times to test TXFULL before breaking out of loop */
    unsigned int retryCount = PUTCHAR_RETRIES;
    
    /* wait while TXFULL is set and retry count is greater than zero */
    while((*uts_reg & UTS_TXFULL) && (retryCount > 0))
        retryCount--;

    /* check to see if we ran out of retries */
    if(retryCount==0)
        return -1;

    /* TXFULL is clear, now write the data into UTXD */
    *utxd_reg = (unsigned char)c;
#endif /* LBL_DEBUG */

    return c;
}

/* *********************************************************************
 * putchar
 *
 * Write a single character to the output device. This function operates
 * on ints for compatibility with the C standard library version of putchar.
 *
 * If the character to be written is a new line ('\n'), putchar will write
 * a carriage return ('\r') before writing the new line.
 *
 * Parameters:
 *   c - character to be written. Is cast to unsigned char.
 *
 * Return Value:
 *   Character written to output device or -1 on error.
 *
 * ********************************************************************/
int putchar(int c)
{
    if(c == NEWLINE)
        putchar_raw(CARRIAGERET);

    return putchar_raw(c);
}

