/*
 * boot_info.h: BOOT_INFO structure.
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
 * 14-Dec-2006  Motorola        Name change for battery presence.
 * 01-May-2007  Motorola        Add new fields, change size of logo version.
 */

#ifndef BOOT_INFO_H
#define BOOT_INFO_H

/* We no longer support V1 of the boot_info structure. */
#define BOOT_INFO_V1		(1)
#define BOOT_INFO_V2		(2)
#define BOOT_INFO_V3		(3)
#define BOOT_INFO_V4		(4)
#define BOOT_INFO_V5		(5)
#define BOOT_INFO_V6		(6)
#define BOOT_INFO_V7		(7)
#define BOOT_INFO_V8		(8)
#define BOOT_INFO_V9		(9)
#define BOOT_INFO_V10		(10)
#define BOOT_INFO_MINVERSION	BOOT_INFO_V2

#define BOOT_INFO_B_VERSION     BOOT_INFO_V10

#define BOOT_INFO_BARKER	(0x13579ACE)
#define BOOT_INFO_INVALID_ADDR	(0xFFFFFFFF)

#define MOT_LOGO_VERSION_SIZE_OLD	24

#ifndef __ASSEMBLY__

#include "types.h"

/*
 * Use this structure for all versions less than BOOT_INFO_B_VERSION.
 * The BOOT_INFO_A structure is frozen.  Don't add any new fields.
 */
typedef struct
{
    uint32_t    ipu_buffer_address;				/* V3 */
    uint32_t    is_ipu_initialized;                             /* V3 */
    uint32_t    gpu_context;                                    /* V3 */
    uint32_t    mbm_version;                                    /* V5 */
    uint32_t    mbm_loader_version;                             /* V5 */
    uint32_t    usb_firmware_address;                           /* V5 */
    uint32_t    usb_firmware_size;                              /* V5 */
    uint32_t    boardid;                                        /* V6 */
    uint32_t    flat_dev_tree_address;                          /* V7 */
    uint32_t    ram_disk_start_address;                         /* V7 */
    uint32_t    ram_disk_size;                                  /* V7 */
    uint32_t    flashing_completed;                             /* V7 */
    uint8_t     logo_version[MOT_LOGO_VERSION_SIZE_OLD];        /* V7 */
    uint16_t    memory_type;                                    /* V8 */
    uint16_t    battery_status_at_boot;                         /* V8 */
    uint32_t    boot_frequency;                                 /* V8 */
    uint32_t    medl_panel_tag_id;                              /* V9 */
    uint32_t    medl_panel_pixel_format;                        /* V9 */
    uint32_t    medl_panel_status;                              /* V9 */
} BOOT_INFO_A;

/*
 * Use this structure for all versions BOOT_INFO_B_VERSION or higher.
 */
typedef struct
{
    uint32_t    ipu_buffer_address;                             /* V3 */
    uint32_t    is_ipu_initialized;                             /* V3 */
    uint32_t    gpu_context;                                    /* V3 */
    uint32_t    mbm_version;                                    /* V5 */
    uint32_t    mbm_loader_version;                             /* V5 */
    uint32_t    usb_firmware_address;                           /* V5 */
    uint32_t    usb_firmware_size;                              /* V5 */
    uint32_t    boardid;                                        /* V6 */
    uint32_t    flat_dev_tree_address;                          /* V7 */
    uint32_t    ram_disk_start_address;                         /* V7 */
    uint32_t    ram_disk_size;                                  /* V7 */
    uint32_t    flashing_completed;                             /* V7 */
    uint16_t    memory_type;                                    /* V8 */
    uint16_t    battery_status_at_boot;                         /* V8 */
    uint32_t    boot_frequency;                                 /* V8 */
    uint32_t    medl_panel_tag_id;                              /* V9 */
    uint32_t    medl_panel_pixel_format;                        /* V9 */
    uint32_t    medl_panel_status;                              /* V9 */
    uint32_t    mbm_bootup_time;                                /* V10 */
    uint32_t    bp_loader_version;                              /* V10 */
    uint32_t    logo_version_max_length;                        /* V10 */
    uint8_t *   logo_version;                                   /* V10 */
    uint32_t    cli_logo_version_max_length;                    /* V10 */
    uint8_t *   cli_logo_version;                               /* V10 */
} BOOT_INFO_B;

/*
 * The BOOT_INFO structure is the structure provided by the MBM when
 * it passes control to the Linux Boot Loader.
 *
 * Starting with version 10 of the BOOT_INFO structure, the size of
 * the logo_version field changed.  This change was unlike all others
 * in that the structure was not just extended with new fields.  In
 * order to handle the new structure, we introduce a union and let the
 * version field determine which to use.
 */
typedef struct
{
    uint32_t    boot_info_version;                              /* V2 */
    uint32_t    boot_info_barker;                               /* V2 */
    uint32_t    powerup_reason;                                 /* V2 */
    uint32_t    kernel_config_address;                          /* V2 */
    uint32_t    kernel_config_size;                             /* V2 */
    uint32_t    kernel_address;                                 /* V2 */
    uint32_t    kernel_size;	                                /* V2 */
    uint32_t    os_loader_load_address;                         /* V2 */
    union {
	BOOT_INFO_A     a;
	BOOT_INFO_B     b;
    } un_bi;
} BOOT_INFO;

#define BI_A un_bi.a
#define BI_B un_bi.b


/*
 * The BIO macro gives an efficient way of getting the offset of a
 * given field in the BOOT_INFO structure.  Because many of the fields
 * are at the same offset, we check first if the offsets are the same.
 * If they are the same, then the compiler can make the decision on
 * which union to use.  Otherwise, the check for boot_info_version
 * is made and the appropriate union chosen for accessing the given
 * field.
 *
 * This macro is only useful for fields that exist in both BOOT_INFO_A
 * and BOOT_INFO_B.  If a field only exists in BOOT_INFO_B, then
 * access it directly.
 */
#define BIO(p,field) \
        ((offsetof(BOOT_INFO,BI_A.field) == offsetof(BOOT_INFO,BI_B.field)) ? \
          (offsetof(BOOT_INFO,BI_A.field)) : \
          (((p)->boot_info_version >= BOOT_INFO_B_VERSION) ? \
           (offsetof(BOOT_INFO,BI_B.field)) : \
           (offsetof(BOOT_INFO,BI_A.field))))

/*
 * BIP returns a properly casted pointer to the given field (field) in
 * the given structure pointer (p).
 */
#define BIP(p,field) \
        ((typeof((p)->BI_B.field) *)(((void *)(p)) + BIO(p,field)))

/*
 * BIF returns the given field (field) in the given structure pointer
 * (p).  This doesn't work for arrays.  Use the BIP macro instead.
 */
#define BIF(p,field) \
        (*((typeof((p)->BI_B.field) *)(((void *)(p)) + BIO(p,field))))


extern BOOT_INFO *boot_infop;

#endif  /* __ASSEMBLY__ */

#endif  /* BOOT_INFO_H */
