/*
 * util.h: Utility include file for the Linux Boot Loader (lbl)
 *
 * Copyright 2006-2007 Motorola, Inc.
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
 * 01-May-2007  Motorola        Include stddef.h.
 */

#ifndef UTIL_H
#define UTIL_H

#include "types.h"

#ifndef __ASSEMBLY__

#include <stddef.h>
#include <stdarg.h>

void *  memcpy(void *dest, const void *src, size_t n);
int     memcmp(const void *dest, const void *src, size_t n);
void *  memset(void *start, int val, size_t n); 

int     strlen(const char *s);

int     sprintf(char * buf, const char *fmt, ...);
int     printf(const char *fmt, ...);

extern int vsprintf(char * buf, const char *fmt, va_list args);

/* putchar is defined in the mach-specific directory */
extern int putchar(int c);
extern int puts(const char *s);

#endif /* __ASSEMBLY__ */

#define PRINTF_BUFFER_SIZE  1024

#define NEWLINE             ('\n')
#define CARRIAGERET         ('\r')

#endif  /* UTIL_H */
