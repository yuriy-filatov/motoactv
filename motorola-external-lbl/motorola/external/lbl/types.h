/*
 * types.h: Useful types needed for the Linux Boot Loader (lbl)
 *
 * Copyright 2006-2007 2009 Motorola, Inc.
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
 * 01-May-2007  Motorola        Remove unncecessary types.
 * 13-Aug-2009  Motorola        Add new type definition.
 */

#ifndef LBL_TYPES
#define LBL_TYPES

#ifndef __ASSEMBLY__

/* Handy type definitions
 * NOTE: The size of these typedefs MUST be identical between the
 *       kernel running on the host machine and the cross-compiled kernel.
 */

typedef signed long         s32;
typedef signed short        s16;
typedef signed char         s8;
typedef unsigned long long  u64;
typedef unsigned long       u32;
typedef unsigned short      u16;
typedef unsigned char       u8;

typedef u32	uint32_t;
typedef u32	u_int32_t;
typedef s32	int32_t;
typedef u16	uint16_t;
typedef u8	uint8_t;


#endif  /* __ASSEMBLY__ */

#endif  /* LBL_TYPES */
