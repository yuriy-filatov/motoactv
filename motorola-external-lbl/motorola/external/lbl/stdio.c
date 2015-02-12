/*
 * stdio.c: C stdio-compatible funtions for writing to serial port.
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
#include "util.h"
#include "mach-zas/mach-debug.h"

/* *********************************************************************
 * puts
 *
 * Write a null-terminated string to the output device.
 *
 * Parameters:
 *   s - The null terminated string that will be written.
 *
 * Return Value:
 *   The number of characters from the string s written to the output
 *   device. (Count does not include any carriage returns that were
 *   automatically added to the output.)
 *
 * Note:
 *   Only the first 1KB of the string will be sent to the output
 *   device.
 *
 * ********************************************************************/
int puts(const char *s)
{
    int i = 0, send_limit=PUTS_SEND_LIMIT;
    
    for(; (*s != '\0')&&(send_limit>0); s++, send_limit--) {
        if(putchar(*s) != -1) {
            i++;
        }
    }

    return i;
}
