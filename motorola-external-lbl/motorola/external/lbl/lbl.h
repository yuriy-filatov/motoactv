/*
 * lbl.h: main include file for the Linux Boot Loader (lbl)
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
 * 18-Jan-2007  brtx43          Modify the definitaion of LBL_PANIC_IF. 
 */

#ifndef LBL_H
#define LBL_H

#define LBL_BARKER	(0xDEADCAFE)
#define LBL_V1		1
#define LBL_V2		2
#define LBL_V3		3
#define LBL_V4          4
#define LBL_VERSION	LBL_V4

/*
 * LBL_VERSION_STR is included in the code so we can tell what version
 * of the LBL we have (use strings on the image).
 * In case you're wondering, both str and xstr are necessary.
 * Try it if you don't believe me and see what you get.
 */
#define str(s)		   #s
#define xstr(s)		   str(s)
#define LBL_VERSION_STR(n) xstr(LBL_VERS= ## n)

/* The arch value provided to the kernel */
#define ARCH_NUMBER 2196

#define howmany(x, y)     (((x)+((y)-1))/(y))
#define roundup(x, y)     ((((x)+((y)-1))/(y))*(y))

/* Convert a tag size in words to bytes. */
#define tagsize2bytes(x)  ((x) << 2)

/* Generate Linux Boot Loader panic codes. */
#define genpanicid(x)			(((0xdead) << 16) | (x))

/* Linux Boot Loader panic codes */
#define LBL_PANIC_MAIN_RETURNED			genpanicid(1)
#define LBL_PANIC_BAD_ADDR			genpanicid(2)
#define LBL_PANIC_BAD_ATAGS			genpanicid(3)
#define LBL_PANIC_KERNEL_RETURNED		genpanicid(4)
#define LBL_PANIC_RELOC_OVERLAP			genpanicid(5)
#define LBL_PANIC_BAD_BOOT_INFO_PTR		genpanicid(6)
#define LBL_PANIC_NO_BOOT_INFO_BARKER		genpanicid(7)
#define LBL_PANIC_DYNLINK_FAILED		genpanicid(8)
#define LBL_PANIC_BAD_BOOT_INFO_VERSION		genpanicid(9)

#ifdef __ASSEMBLY__

#ifdef LBL_DEBUG

#define LBL_PANIC(code, msg) \
        ldr     r0, =code; \
        adr     r1, msg; \
        b       lbl_fail;

#define LBL_PANIC_IF(test, code, msg) \
        ldr ##test r0, =code; \
        adr ##test r1, msg; \
        b   ##test lbl_fail; 

#else /* LBL_DEBUG */

#define LBL_PANIC(code, msg) \
        ldr     r0, =code; \
        b       lbl_fail;

#define LBL_PANIC_IF(test, code, msg) \
        ldr ## test r0, =code; \
        b ##test    lbl_fail;

#endif /* LBL_DEBUG */

#else /* __ASSEMBLY__ */


#ifdef LBL_DEBUG

#define LBL_PANIC(code, msg) \
        lbl_fail(code, msg)

#else /* not defined LBL_DEBUG */

#define LBL_PANIC(code, msg) \
        lbl_fail(code, 0)

#endif /* LBL_DEBUG */

/* Function prototypes */
int process_atags(BOOT_INFO *boot_infop);
void lbl_fail(u32 panicid, const char *msg);
int validate_addr(u32 addr);

#endif  /*  __ASSEMBLY__ */

#endif  /* LBL_H */
