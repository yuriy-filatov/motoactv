/*
 * atag.c
 *
 * Process the ATAGs provided via the BOOT_INFO structure and write
 * them out with the new/merged information at the location where the
 * kernel expects them.
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
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Initial revision.
 * 09-Nov-2006  Motorola        Don't look for setup.h in current directory
 * 14-Dec-2006  Motorola        Changed ATAG name for battery presence.
 * 01-May-2007  Motorola        Added new ATAGs, changed logo version size.
 */

#include "boot_info.h"

/*
 * #include <asm/setup.h>
 * Switch to use old setup.h, to get definition for tag_powerup_reason
 */
#include "setup.h"

#include "util.h"
#include "lbl.h"

static int found_initrd2 = 0;

/* Swap endianness of a word. */
#define LBLSWAB32(x) \
({ \
    u32 __x = (x); \
    ((u32)( \
        (((u32)(__x) & (u32)0x000000ffUL) << 24) | \
        (((u32)(__x) & (u32)0x0000ff00UL) <<  8) | \
        (((u32)(__x) & (u32)0x00ff0000UL) >>  8) | \
        (((u32)(__x) & (u32)0xff000000UL) >> 24) )); \
})

/**************************************************************
 * process_atags
 *
 * Parse the binary atag parameter block and read the TAG attributes
 * to determine what type of entries are present in the ATAG.  Then
 * pass the params tag to the appropriate handler function.
 *
 * Parameters:
 *         boot_infop - pointer to the BOOT_INFO struct
 * Return Value:
 *         0 - success, 1 - failure
 **************************************************************/
int process_atags(BOOT_INFO *boot_infop)
{
    struct tag *tp;
    struct tag *nexttp;
    int size = 0;
    struct tag *logo_version_tp = 0;
    struct tag *cli_logo_version_tp = 0;
    char *cp;

    tp = (struct tag *)(boot_infop->kernel_config_address);
    
    /*
     * We walk through the ATAGs and add the Motorola specific ATAGs
     * at the end.
     */
    while (tp->hdr.tag != ATAG_NONE) {
        switch (tp->hdr.tag) {
        case ATAG_CORE:
        case ATAG_MEM:
        case ATAG_VIDEOTEXT:
        case ATAG_RAMDISK:
        case ATAG_INITRD2:
        case ATAG_SERIAL:
        case ATAG_REVISION:
        case ATAG_VIDEOLFB:
        case ATAG_MEMCLK:
        case ATAG_CMDLINE:
            nexttp = tag_next(tp);
	    size += ((u32)nexttp - (u32)tp);
	    if (size > COMMAND_LINE_SIZE) {
		return 1;
	    }
	    if (tp->hdr.tag == ATAG_INITRD2) {
		found_initrd2 = 1;
	    }
	    tp = nexttp;
            break;


        default:
            return 1;
            break;
        }
    }

    /*
     * Add Motorola specific ATAGs to end of the ATAGs list
     */
    size += tagsize2bytes(tag_size(tag_powerup_reason));
    if (size > COMMAND_LINE_SIZE) {
	return 1;
    }
    tp->hdr.tag = ATAG_POWERUP_REASON;
    tp->hdr.size = tag_size(tag_powerup_reason);
    tp->u.powerup_reason.powerup_reason = boot_infop->powerup_reason;
    tp = tag_next(tp);
    
    if (boot_infop->boot_info_version >= BOOT_INFO_V3) {
        /*
         * Add ipu_buffer_address atag to the list of ATAGs
         */
        size += tagsize2bytes(tag_size(tag_ipu_buffer_address));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_IPU_BUFFER_ADDRESS;
        tp->hdr.size = tag_size(tag_ipu_buffer_address);
        tp->u.ipu_buffer_address.ipu_buffer_address = BIF(boot_infop,ipu_buffer_address);
        tp = tag_next(tp);

        /*
         * Add is_ipu_initialized atag to the list of ATAGs
         */
        size += tagsize2bytes(tag_size(tag_is_ipu_initialized));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_IS_IPU_INITIALIZED;
        tp->hdr.size = tag_size(tag_is_ipu_initialized);
	tp->u.is_ipu_initialized.is_ipu_initialized = BIF(boot_infop, is_ipu_initialized);
        tp = tag_next(tp);

        /*
         * Add gpu_context atag to the list of ATAGs
         */
        size += tagsize2bytes(tag_size(tag_gpu_context));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_GPU_CONTEXT;
        tp->hdr.size = tag_size(tag_gpu_context);
        tp->u.gpu_context.gpu_context = BIF(boot_infop, gpu_context);
	tp = tag_next(tp);
    }

    if (boot_infop->boot_info_version >= BOOT_INFO_V5) {
        /*
         * Add usb hs firmware address atag to the list of ATAGs
         */
        size += tagsize2bytes(tag_size(tag_usb_firmware_address));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_USB_FIRMWARE_ADDRESS;
        tp->hdr.size = tag_size(tag_usb_firmware_address);
         tp->u.usb_firmware_address.usb_firmware_address =
                 BIF(boot_infop, usb_firmware_address);
        tp = tag_next(tp);

        /*
         * Add usb hs firmware partition size atag to the list of ATAGs
         */
        size += tagsize2bytes(tag_size(tag_usb_firmware_size));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_USB_FIRMWARE_SIZE;
        tp->hdr.size = tag_size(tag_usb_firmware_size);
        tp->u.usb_firmware_size.usb_firmware_size = 
	BIF(boot_infop,usb_firmware_size);
        tp = tag_next(tp);

        /*
         * Add MBM version to the list of ATAGs.
         */
        size += tagsize2bytes(tag_size(tag_mbm_version));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_MBM_VERSION;
        tp->hdr.size = tag_size(tag_mbm_version);
        tp->u.mbm_version.mbm_version = BIF(boot_infop,mbm_version);
        tp = tag_next(tp);

        /*
         * Add MBM loader version to the list of ATAGs.
         */
        size += tagsize2bytes(tag_size(tag_mbm_loader_version));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_MBM_LOADER_VERSION;
        tp->hdr.size = tag_size(tag_mbm_loader_version);
        tp->u.mbm_loader_version.mbm_loader_version =
			BIF(boot_infop,mbm_loader_version);
        tp = tag_next(tp);

    }

    if (boot_infop->boot_info_version >= BOOT_INFO_V6) {
        /*
         * Add boardid atag to the list of ATAGs.
         */
        size += tagsize2bytes(tag_size(tag_boardid));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_BOARDID;
        tp->hdr.size = tag_size(tag_boardid);
        tp->u.boardid.boardid = BIF(boot_infop,boardid);
        tp = tag_next(tp);
    }

    if (boot_infop->boot_info_version >= BOOT_INFO_V7) {
        /*
         * Add flat_dev_tree_address atag to the list of ATAGs.
         */
        u32 * addr = (u32 *) BIF(boot_infop,flat_dev_tree_address);
        size += tagsize2bytes(tag_size(tag_flat_dev_tree_address));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_FLAT_DEV_TREE_ADDRESS;
        tp->hdr.size = tag_size(tag_flat_dev_tree_address);
        tp->u.flat_dev_tree_address.flat_dev_tree_address = (u32)addr;

        /* Get size from header of serialized device tree. */
        tp->u.flat_dev_tree_address.flat_dev_tree_size = 0;
        if (addr != (u32 *)0 && addr != (u32 *)0xFFFFFFFF) {
            if (*addr == FLATTREE_BEGIN_SERIALIZED)
                tp->u.flat_dev_tree_address.flat_dev_tree_size = *(addr+1);
            else if (*addr == FLATTREE_BEGIN_SERIALIZED_OTHEREND)
                tp->u.flat_dev_tree_address.flat_dev_tree_size = LBLSWAB32(*(addr+1));
        }
        tp = tag_next(tp);

        /*
         * Add the ramdisk atag to the list of ATAGs, but only if
	 * the values indicate that a ramdisk is present.
         */
        if (BIF(boot_infop,ram_disk_size) != 0)
	{
	    /* The MBM is passing us a ramdisk to use. */
            if (found_initrd2) {
                /* Fail if we have already seen ATAG_INITRD2. */
                return 1;
            }
            size += tagsize2bytes(tag_size(tag_initrd));
            if (size > COMMAND_LINE_SIZE)
                return 1;
            tp->hdr.tag = ATAG_INITRD2;
            tp->hdr.size = tag_size(tag_initrd);
    
            tp->u.initrd.start = BIF(boot_infop,ram_disk_start_address);
            tp->u.initrd.size = BIF(boot_infop,ram_disk_size);
            tp = tag_next(tp);
	}

        /*
         * Add the flashing_completed atag to the list of ATAGs.
         */
        size += tagsize2bytes(tag_size(tag_flashing_completed));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_FLASHING_COMPLETED;
        tp->hdr.size = tag_size(tag_flashing_completed);
        tp->u.flashing_completed.flashing_completed = 
		BIF(boot_infop,flashing_completed);
        tp = tag_next(tp);

        /*
         * Add the logo_version atag to the list of ATAGs.
         */
	if(boot_infop->boot_info_version >= BOOT_INFO_V10)
	{
        	size += tagsize2bytes(tag_size(tag_logo_version));
        	if (size > COMMAND_LINE_SIZE)
            	return 1;
        	tp->hdr.tag = ATAG_LOGO_VERSION;
        	tp->hdr.size = tag_size(tag_logo_version);
		tp->u.logo_version.logo_version = boot_infop->BI_B.logo_version;
		tp->u.logo_version.logo_version_max_length =
			boot_infop->BI_B.logo_version_max_length;
		logo_version_tp = tp;
	}
	else
	{
		size += tagsize2bytes(tag_size(tag_logo_version));
		if (size > COMMAND_LINE_SIZE)
			return 1;
		tp->hdr.tag = ATAG_LOGO_VERSION;
		tp->hdr.size = tag_size(tag_logo_version);
		memcpy (tp->u.logo_version.logo_version_string, 
			BIP(boot_infop, logo_version), MOT_LOGO_VERSION_SIZE_OLD);
		tp->u.logo_version.logo_version_max_length = MOT_LOGO_VERSION_SIZE_OLD;
		tp->u.logo_version.logo_version = NULL;
	}
        tp = tag_next(tp);
    }

    if (boot_infop->boot_info_version >= BOOT_INFO_V8) {
        /*
         * Add memory_type atag to the list of ATAGs.
         */
        size += tagsize2bytes(tag_size(tag_memory_type));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_MEMORY_TYPE;
        tp->hdr.size = tag_size(tag_memory_type);
        tp->u.memory_type.memory_type = BIF(boot_infop,memory_type);
        tp = tag_next(tp);

        /*
         * Add battery_status_at_boot atag to the list of ATAGs.
         */
        size += tagsize2bytes(tag_size(tag_battery_status_at_boot));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_BATTERY_STATUS_AT_BOOT;
        tp->hdr.size = tag_size(tag_battery_status_at_boot);
        tp->u.battery_status_at_boot.battery_status_at_boot =
			BIF(boot_infop,battery_status_at_boot);
        tp = tag_next(tp);

        /*
         * Add boot_frequency atag to the list of ATAGs.
         */
        size += tagsize2bytes(tag_size(tag_boot_frequency));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_BOOT_FREQUENCY;
        tp->hdr.size = tag_size(tag_boot_frequency);
        tp->u.boot_frequency.boot_frequency =
			BIF(boot_infop,boot_frequency);
	 tp = tag_next(tp);
    }
   
    if (boot_infop->boot_info_version >= BOOT_INFO_V9) {
        /*
         * Add medl_panel_info atag to the list of ATAGs.
         */
         size += tagsize2bytes(tag_size(tag_medl_info));
         if (size > COMMAND_LINE_SIZE)
             return 1;
         tp->hdr.tag = ATAG_MEDL_INFO;
         tp->hdr.size = tag_size(tag_medl_info);
         tp->u.medl_info.medl_panel_tag_id =
                         BIF(boot_infop, medl_panel_tag_id);
         tp->u.medl_info.medl_panel_pixel_format =
                         BIF(boot_infop, medl_panel_pixel_format);
         tp->u.medl_info.medl_panel_status =
                         BIF(boot_infop, medl_panel_status);
         tp = tag_next(tp);
     }

    if (boot_infop->boot_info_version >= BOOT_INFO_V10) {
       /*
        * Add mbm_bootup_time atag to the list of ATAGs.
        */
        size += tagsize2bytes(tag_size(tag_mbm_bootup_time));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_MBM_BOOTUP_TIME;
        tp->hdr.size = tag_size(tag_mbm_bootup_time);
		tp->u.mbm_bootup_time.mbm_bootup_time =
                       boot_infop->BI_B.mbm_bootup_time;

    	tp = tag_next(tp);

        /*
	 * Add bp_loader_version atag to the list of ATAGs.
        */
        size += tagsize2bytes(tag_size(tag_bp_loader_version));
        if (size > COMMAND_LINE_SIZE)
             return 1;
        tp->hdr.tag = ATAG_BP_LOADER_VERSION;
        tp->hdr.size = tag_size(tag_bp_loader_version);
        tp->u.bp_loader_version.bp_loader_version =
                        boot_infop->BI_B.bp_loader_version;

        tp = tag_next(tp);
 
        /*
         * Add cli_logo_version atag to the list of ATAGs.
         */
        size += tagsize2bytes(tag_size(tag_cli_logo_version));
        if (size > COMMAND_LINE_SIZE)
            return 1;
        tp->hdr.tag = ATAG_CLI_LOGO_VERSION;
        tp->hdr.size = tag_size(tag_cli_logo_version);
        tp->u.cli_logo_version.cli_logo_version =
                        boot_infop->BI_B.cli_logo_version;
        tp->u.cli_logo_version.cli_logo_version_max_length =
                        boot_infop->BI_B.cli_logo_version_max_length;
        cli_logo_version_tp = tp;

        tp = tag_next(tp);
    }


    /*
     * The last ATAG is ATAG_NONE.
     */
    size += sizeof(struct tag_header);
    if (size > COMMAND_LINE_SIZE) {
        return 1;
    }
    tp->hdr.tag = ATAG_NONE;
    tp->hdr.size = 0;
	    cp = ((char *)tp) + sizeof(struct tag_header);
    *cp = ' ';	/* Initialize with space in case there is no logo version. */

    /*
     * Copy the logo version and cli logo version from the location
     * provided by the MBM to just after the last ATAG.  The physical
     * address chosen by the MBM may not be easily mappable during the
     * early Linux boot, so we copy it to a place known to be mapped.
     */
    if (logo_version_tp)
    {
	if (logo_version_tp->u.logo_version.logo_version_max_length == 0)
	    size++;
	else
	    size += logo_version_tp->u.logo_version.logo_version_max_length;
	if (size > COMMAND_LINE_SIZE)
	    return 1;
	memcpy(cp, logo_version_tp->u.logo_version.logo_version,
	       logo_version_tp->u.logo_version.logo_version_max_length);
	logo_version_tp->u.logo_version.logo_version = cp;
	if (logo_version_tp->u.logo_version.logo_version_max_length == 0)
	    cp++;
	else
	    cp += logo_version_tp->u.logo_version.logo_version_max_length;
    }

    *cp = ' ';	/* Initialize with space in case there is no CLI logo version. */
    if (cli_logo_version_tp)
    {
	if (cli_logo_version_tp->u.cli_logo_version.cli_logo_version_max_length == 0)
	    size++;
	else
	    size += cli_logo_version_tp->u.cli_logo_version.cli_logo_version_max_length;
	if (size > COMMAND_LINE_SIZE)
	    return 1;
	memcpy(cp, cli_logo_version_tp->u.cli_logo_version.cli_logo_version,
               cli_logo_version_tp->u.cli_logo_version.cli_logo_version_max_length);
	cli_logo_version_tp->u.cli_logo_version.cli_logo_version = cp;
	if (cli_logo_version_tp->u.cli_logo_version.cli_logo_version_max_length == 0)
	    cp++;
	else
	    cp += cli_logo_version_tp->u.cli_logo_version.cli_logo_version_max_length;
    }

    return 0;
}
