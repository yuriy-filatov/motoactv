/*
 * initdebug.S: setup debugging support in LBL. For ZAS output goes to UART3.
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
 * _init_debug
 *
 * For Zeus/Argon+/SCM-A11, debugging output goes to UART3.
 *
 * Setup UART for 115200bps 8-bits, no parity, 1 stop bit, all interrupts
 * disabled.
 *
 * Parameters:
 *   r0 - The base address of the UART to initialize.
 * Return Value:
 *   None.
 *
 * Side Effects:
 *   r1 is destroyed.
 *
 * ********************************************************************/
.globl _init_debug
_init_debug:
        /*
         * Enable the UART. 
         */
        ldr     r1, =(UCR1_UARTEN)
        strh    r1, [r0, #UCR1]

        /*
         * Clear out UCR2 during configuration.
         */
        ldr     r1, =0x0
        strh    r1, [r0, #UCR2]

        /*
         * Setup UCR2:
         *    ESCI: 0 -- Disable escape sequence interrupt
         *    IRTS: 1 -- Ignore the RTS pin
         *    CTSC: 0 -- ipp_uart_cts_b pin is controlled by CTS bit
         *    CTS:  0 -- ipp_uart_cts_b pin is high (inactive)
         *    ESCEN: 0 -- disable escape sequence detection logic
         *    RTEC: 00 -- Request to Send Edge Control: trigger on rising edge
         *    PREN: 0 -- No parity
         *    PROE: 0 -- even parity (no effect since PREN is 0)
         *    STPB: 0 -- 1 stop bit
         *    WS:   1 -- 8 bit word size
         *    RTSEN: 0 -- Disable RTS interrupt
         *    ATEN: 0 -- Disable aging timer
         *    TXEN: 1 -- enable the transmitter
         *    RXEN: 1 -- Enable the receiver
         *    SRST: 1 -- Disable software reset
         */
        ldr     r1, =(UCR2_IRTS|UCR2_WS|UCR2_TXEN|UCR2_RXEN|UCR2_SRST)
        strh    r1, [r0, #UCR2]

        /*
         * Setup UCR3:
         *    DEPC: 00 -- DTR/DSR Interrupt Edge Control. (disabled elsewhere)
         *    DTREN: 0 -- Disable DTR interrupt
         *    PARERREN: 0 -- Disable parity error interrupt
         *    FRAERREN: 0 -- Disable frame error interrupt
         *    DSR: 1 -- ipp_uart_dsr_dce_o_b set to logical 1
         *              Indicates to remote modem that UART is operational.
         *    DCD: 1 -- ipp_uart_dcd_dce_o_b set to logical 1
         *              Indicates to remote device that a good carrier is being
         *              received from the UART.
         *    RI: 1 -- ipp_uart_ri_dce_o_b set to logical 1
         *             Indicates to the remote device that UART is detecting a
         *             ringing tone.
         *    ADNIMP: 0 -- New auto-baud rate detection enabled (ignored)
         *    RXDSEN: 0 -- Disable receive status interrupt
         *    AIRINTEN: 0 -- Disble AIRINT interrupt
         *    AWAKEN: 0 -- Disable async, wake interrupt
         *    DTRDEN: 0 -- Disable DTRD interrupt
         *    RXDMUXSEL: 0 -- RXD serial input pin
         *    INVT: 0 -- Active low infrared transmission (ignored)
         *    ACIEN: 0 -- ACTS interrupt disabled
         */
        ldr     r1, =(UCR3_DSR|UCR3_DCD|UCR3_RI)
        strh    r1, [r0, #UCR3]

        /*
         * Setup UCR4:
         *    CTSTL: 100000 -- CTS trigger level (ignored due to UCR2_CTSC)
         *    INVR: 0 -- active low infrared detection (ignored)
         *    ENIRI: 0 -- Disable serial IR interrupt
         *    WKEN: 0 -- Disable WAKE interrupt
         *    IDDMAEN: 0 -- disable idle DMA interrupt
         *    IRSC: 0 -- IR special case (ignored)
         *    LPBYP: 0 -- Low power features disabled
         *    TCEN: 0 -- Transmit control interript disabled
         *    BKEN: 0 -- Disabled BRCD interrupt
         *    OREN: 0 -- disable receiver overrun interrupt
         *    DREN: 0 -- disable RDR interrupt
         */
        ldr     r1, =(UCR4_CTSTL(32))
        strh    r1, [r0, #UCR4]

        /*
         * Setup UFCR:
         *    TXTL: 000010 -- TRDY interrupt trigger level (ignored)
         *    RFDIV: 101 -- Set reference clock divisor. 0x5 = 1
         *    DCEDTE: 0 -- DCE mode
         *    RXTL: 000001 -- RRDY interrupt trigger level (ignored)
         */
        ldr     r1, =(UFCR_TXTL(2)|UFCR_RFDIV_ONE|UFCR_RXTL(1))
        strh    r1, [r0, #UFCR]

        /*
         * Setup UESC:
         *    Escape character is 0x2B (ESC)
         */
        ldr     r1, =0x2b
        strh    r1, [r0, #UESC]

        /*
         * Setup UTIM:
         *    Maximum time in ms between escape characters: 0x00 = 2ms
         */
        ldr     r1, =0x00
        strh    r1, [r0, #UTIM]

#ifdef MACH_SCMA11REF
        /*
         * Set the baud rate. UBIR must be written before UBMR.
         *
         * Reference clock = 26Mhz
         * Reference clock divisor (UFCR_RFDIV) = 1
         *
         * 115200 baud = (26 Mhz / UFCR_RFDIV ) / ( 16 * (UBMR+1)/(UBIR+1))
         */
        ldr     r1, =0x023f
        strh    r1, [r0, #UBIR]

        ldr     r1, =0x1fbc
        strh    r1, [r0, #UBMR]
#else /* !MACH_SCMA11REF */
#ifdef MACH_BUTEREF
        /*
         * Set the baud rate. UBIR must be written before UBMR.
         *
         * Reference clock = 66.5Mhz
         * Reference clock divisor (UFCR_RFDIV) = 1
         *
         * 115200 baud = (66.5 Mhz / UFCR_RFDIV ) / ( 16 * (UBMR+1)/(UBIR+1))
         */
        ldr     r1, =0x01f3
        strh    r1, [r0, #UBIR]

        ldr     r1, =0x467b
        strh    r1, [r0, #UBMR]
#else /* !MACH_BUTEREF */
#ifdef MACH_ZEUSEVB
        /*
         * Set the baud rate. UBIR must be written before UBMR.
         *
         * Reference clock = 16.8Mhz
         * Reference clock divisor (UFCR_RFDIV) = 1
         *
         * 115200 baud = (16.8 Mhz / UFCR_RFDIV ) / ( 16 * (UBMR+1)/(UBIR+1))
         */
        ldr     r1, =0x047f
        strh    r1, [r0, #UBIR]

        ldr     r1, =0x2903
        strh    r1, [r0, #UBMR]
#else
#error Baud-rate not known for this machine type.
#endif /* !MACH_ZEUSEVB */
#endif /* !MACH_BUTEREF */
#endif /* !MACH_SCMA11REF */

        /*
         * Setup ONEMS:
         *    Ignore one millisecond timer.
         */
        ldr     r1, =0x0
        strh    r1, [r0, #ONEMS]

        /* Setup UCR1:
         *    ADEN: 0 -- disable automatic detection of baudrate interrupt
         *    ADBR: 0 -- disable automatic detection of baudrate
         *    TRDYEN: 0 -- disable transmitter readt interrupt
         *    IDEN: 0 -- Disable idle interrupt
         *    ICD: 00 -- don't care about idle condition detection
         *    RRDYEN: 0 -- Disable receiver ready iterrupt
         *    RXDMAEN: 0 -- Disable Receiver Ready DMA request
         *    IREN: 0 -- Disable infrared interface enable
         *    TXMPTYEN: 0 -- Disable transmitter empty interrupt
         *    RTSEN: 0 -- Disable RTS delta interrupt
         *    SNDBRK: 0 -- Do not send a break character
         *    TXDMAEN: 0 -- Disable transmit DMA request
         *    ATDMEAN: 0 -- Disable aging DMA timer interrupt
         *    DOZE: 0 -- enable UART in doze state
         *    UARTEN: 1 -- enable the uart
         */
        ldr     r1, =(UCR1_UARTEN)
        /* 0x1 */
        strh    r1, [r0, #UCR1]

        /* return from this function */
        mov     pc, lr
