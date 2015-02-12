/*
 * main.c: main file for the Linux Boot Loader (lbl)
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

#include "types.h"
#include "util.h"
#include "boot_info.h"
#include "lbl.h"

#ifdef LBL_DEBUG
#include "debug.h"
#endif /* LBL_DEBUG */

/* Local prototypes */
static void boot_linux(BOOT_INFO *boot_infop);


/* Global variables */
BOOT_INFO *boot_infop;          /* Ptr to BOOT_INFO struct from MBM */


/**************************************************************
 * main
 *
 * Main program.
 * 
 * Parameters:
 *         None
 * Return Value:
 *         None.  This function does not return.
 **************************************************************/
int main(void)
{
    if (process_atags(boot_infop)) {
        LBL_PANIC(LBL_PANIC_BAD_ATAGS, "Error processing ATAGS.\n");
    }

    boot_linux(boot_infop);
    /* NOTREACHED */
    
    return 0;   /* shuts up compiler warning */
}


/**************************************************************
 * validate_addr
 *
 * Validate the given address.  We look for addresses that are
 * obviously bad and just stop the boot loader.
 * 
 * Parameters:
 *         addr - Address to validate
 * Return Value:
 *         Zero if addr is null or not word-aligned; non-zero
 *         otherwise.
 **************************************************************/
int validate_addr(u32 addr)
{
    return !((addr & 0x3) || (addr == 0));
}

/**************************************************************
 * lbl_fail
 *
 * Stop the Linux Boot Loader.
 * 
 * Parameters:
 *         panicid - panic code
 *         msg - null-terminated string describing the error
 * Return Value:
 *         None.  This function does not return.
 **************************************************************/
void lbl_fail(u32 panicid, const char *msg)
{
#ifdef LBL_DEBUG
    puts(msg);

    /* put the panicid back into r0 */
    asm("mov r0, %0" : : "r"(panicid) : "r0");
#endif /* LBL_DEBUG */

    for (;;)
        /* forever */;

    /* NOTREACHED */
}


/**************************************************************
 * boot_linux
 *
 * Boot linux by jumping to the given address.
 * 
 * Parameters:
 *         kernel_address - Address of the kernel
 * Return Value:
 *         None.  There is no return from this function.
 **************************************************************/
static void boot_linux(BOOT_INFO *boot_infop)
{
    void (*theKernel)(int zero, int arch, uint32_t atags);

    if( !validate_addr((u32)boot_infop->kernel_address) ) {
        LBL_PANIC(LBL_PANIC_BAD_ADDR,
                "Kernel address is null or not word-aligned.\n");
        /* NOTREACHED */
    }

#ifdef LBL_DEBUG
    /* print out some information about the kernel being started */
    printf("LBL: starting kernel at: 0x%08X, powerup reason: 0x%08X\n",
            (u32)boot_infop->kernel_address, (u32)boot_infop->powerup_reason);
#endif /* LBL_DEBUG */

    /* start kernel */
    theKernel = (void (*)(int, int, uint32_t))(boot_infop->kernel_address);

    /*
     * The ARCH_NUMBER (machine type in the kernel) is used by the
     * kernel *only* if multiple architectures are configured, in
     * which case the value is used to decide which machine type to
     * use.  Otherwise, this value is ignored.
     */
    theKernel(0, ARCH_NUMBER, boot_infop->kernel_config_address);

    LBL_PANIC(LBL_PANIC_KERNEL_RETURNED, "Kernel returned control to LBL.\n");
}
