/*
 * genasm.c: program to generate values used in the asm files in
 *           the Linux Boot Loader (lbl)
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

#include <stdio.h>
#include <stddef.h>
#include "boot_info.h"

int
main(int argc, char *argv[])
{
    /*
     * Print the following structure offsets into the BOOT_INFO
     * structure.
     */
    printf ("#define BOOT_INFO_BARKER_OFF (%d)\n",
		offsetof(BOOT_INFO, boot_info_barker));
    printf ("#define BOOT_INFO_VERSION_OFF (%d)\n",
		offsetof(BOOT_INFO, boot_info_version));
    printf ("#define BOOT_INFO_OS_LDR_LOAD_ADDR_OFF (%d)\n",
		offsetof(BOOT_INFO, os_loader_load_address));

    return 0;
}
