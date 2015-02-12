/*
 * dynlink.c: dynamic linker for the Linux Boot Loader (lbl)
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
#include "elf.h"

int
dynlink(u_int32_t loff)
{
    ElfW(Dyn)       *dynp;
    ElfW(Rel)       *relp = 0;
    ElfW(Word)      relsz = 0;
    ElfW(Addr)      *ra = 0;
    int             i;

    /*
     * Locate DT_REL and DT_RELSZ in dynp.  These provide the location
     * of the relocation information (.rel.dyn section).  We'll use
     * the DT_REL and DT_RELSZ to rewrite the relocation information
     * pointed to in the .rel.dyn section.  (This is the .got tbl.).
     */
    dynp = (ElfW(Dyn) *)((long)&_DYNAMIC + loff);
    while (dynp->d_tag) {
        if (dynp->d_tag == DT_REL)
            relp = (ElfW(Rel) *)(dynp->d_un.d_val + loff);
        if (dynp->d_tag == DT_RELSZ)
            relsz = dynp->d_un.d_val;
        dynp++;
    }

    /*
     * If we can't find either the DT_REL or DT_RELSZ, then we return
     * failure.  It would be very unusual for both the DT_REL and
     * DT_RELSZ to be 0.
     */
    if ((relp == 0) || (relsz == 0)) {
        return 1;
    }

    /*
     * Relocate the .got entries.
     */
    for (i=0; i<relsz; i+=sizeof(ElfW(Rel))) {
        /* We should only see R_ARM_RELATIVE entries. */
        if (ELF32_R_TYPE(relp->r_info) != R_ARM_RELATIVE) {
            return 1;
        }
        ra = (ElfW(Addr) *)(relp->r_offset + loff);
        *ra += loff;
        relp++;
    }

    return 0;
}
