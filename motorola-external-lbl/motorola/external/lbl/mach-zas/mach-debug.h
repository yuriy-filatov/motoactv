/*
 * mach-debug.h: Zeus/ArgonPlus/SCM-A11 specific debugging definitions.
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

#ifndef __MACH_DEBUG_H__
#define __MACH_DEBUG_H__

#include "../util.h"

#define UART3_BASE_ADDR (0x5000C000)
#define URXD            (0x00)
#define UTXD            (0x40)
#define UCR1            (0x80)
#define UCR2            (0x84)
#define UCR3            (0x88)
#define UCR4            (0x8C)
#define UFCR            (0x90)
#define USR1            (0x94)
#define USR2            (0x98)
#define UESC            (0x9C)
#define UTIM            (0xA0)
#define UBIR            (0xA4)
#define UBMR            (0xA8)
#define UBRC            (0xAC)
#define ONEMS           (0xB0)
#define UTS             (0xB4)

/* UCR1 control bits */
#define UCR1_ADEN           (0x8000)
#define UCR1_ADBR           (0x4000)
#define UCR1_TRDYEN         (0x2000)
#define UCR1_IDEN           (0x1000)
/* ICD is bits 10 and 11 */
#define UCR1_ICD_FOUR       (0x0000)
#define UCR1_ICD_EIGHT      (0x0400)
#define UCR1_ICD_SIXTEEN    (0x0800)
#define UCR1_ICD_THIRTYTWO  (0x0C00)
#define UCR1_RRDYEN         (0x0200)
#define UCR1_RXDMAEN        (0x0100)
#define UCR1_IREN           (0x0080)
#define UCR1_TXMPTYEN       (0x0040)
#define UCR1_RTSDEN         (0x0020)
#define UCR1_SNDBRK         (0x0010)
#define UCR1_TXDMAEN        (0x0008)
#define UCR1_ATDMAEN        (0x0004)
#define UCR1_DOZE           (0x0002)
#define UCR1_UARTEN         (0x0001)

/* UCR2 control bits */
#define UCR2_ESCI           (0x8000)
#define UCR2_IRTS           (0x4000)
#define UCR2_CTSC           (0x2000)
#define UCR2_CTS            (0x1000)
#define UCR2_ESCEN          (0x0800)
/* RTEC is bits 9 and 10 */
#define UCR2_RTEC_RISING    (0x0000)
#define UCR2_RTEC_FALLING   (0x0200)
#define UCR2_RTEC_ANY       (0x0600)
#define UCR2_PREN           (0x0100)
#define UCR2_PROE           (0x0080)
#define UCR2_STPB           (0x0040)
#define UCR2_WS             (0x0020)
#define UCR2_RTSEN          (0x0010)
#define UCR2_ATEN           (0x0008)
#define UCR2_TXEN           (0x0004)
#define UCR2_RXEN           (0x0002)
#define UCR2_SRST           (0x0001)

/* UCR3 control bits */
/* DPEC is bits 14 and 15 */
#define UCR3_DPEC_RISING    (0x0000)
#define UCR3_DPEC_FALLING   (0x4000)
#define UCR3_DPEC_ANY       (0xC000)
#define UCR3_DTREN          (0x2000)
#define UCR3_PARERREN       (0x1000)
#define UCR3_FRAERREN       (0x0800)
#define UCR3_DSR            (0x0400)
#define UCR3_DCD            (0x0200)
#define UCR3_RI             (0x0100)
#define UCR3_ADNIMP         (0x0080)
#define UCR3_RXDSEN         (0x0040)
#define UCR3_AIRINTEN       (0x0020)
#define UCR3_AWAKEN         (0x0010)
#define UCR3_DTRDEN         (0x0008)
#define UCR3_RXDMUXSEL      (0x0004)
#define UCR3_INVT           (0x0002)
#define UCR3_ACIEN          (0x0001)

/* UCR4 control bits */
/* CTSTL is bits 10 through 15; max. level is 32 */
#define UCR4_CTSTL(level)   ((level)<<10)
#define UCR4_INVR           (0x0200)
#define UCR4_ENIRI          (0x0100)
#define UCR4_WKEN           (0x0080)
#define UCR4_IDDMAEN        (0x0040)
#define UCR4_IRSC           (0x0020)
#define UCR4_LPBYP          (0x0010)
#define UCR4_TCEN           (0x0008)
#define UCR4_BKEN           (0x0004)
#define UCR4_OREN           (0x0002)
#define UCR4_DREN           (0x0001)

/* UFCR control bits */
/* TXTL is bits 10 through 15; valid values are 2-32 */
#define UFCR_TXTL(level)    ((level)<<10)
/* RFDIV is bits 7 through 9 */
#define UFCR_RFDIV_SIX      (0x0000)
#define UFCR_RFDIV_FIVE     (0x0080)
#define UFCR_RFDIV_FOUR     (0x0100)
#define UFCR_RFDIV_THREE    (0x0180)
#define UFCR_RFDIV_TWO      (0x0200)
#define UFCR_RFDIV_ONE      (0x0280)
#define UFCR_RFDIV_SEVEN    (0x0300)
#define UFCR_DCEDTE         (0x00400)
/* RXTL is bits 0 through 5; value values are 0-32 */
#define UFCR_RXTL(level)    ((level)&&0xFFC0)

/* UTS status bits */
#define UTS_FRCPERR     (0x2000)
#define UTS_LOOP        (0x1000)
#define UTS_DBGEN       (0x0800)
#define UTS_LOOPIR      (0x0400)
#define UTS_RXDBG       (0x0200)
#define UTS_TXEMPTY     (0x0040)
#define UTS_RXEMPTY     (0x0020)
#define UTS_TXFULL      (0x0010)
#define UTS_RXFULL      (0x0008)
#define UTS_SOFTRST     (0x0001)

/* control the behavior of putchar and puts */
#define PUTCHAR_RETRIES 0x10000000
#define PUTS_SEND_LIMIT PRINTF_BUFFER_SIZE

/*
 * On Zeus/Argon+/SCM-A11 debugging console is on UART3
 */
#define STDOUT_DEVICE   UART3_BASE_ADDR

#endif /* __MACH_DEBUG_H__ */
