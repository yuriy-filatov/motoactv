/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __LINUX_OMAP_DEVICE_TYPE_H__
#define __LINUX_OMAP_DEVICE_TYPE_H__

#define OMAP_DEVICE_TYPE_DRV_NAME "omap_dev_type"

enum {
 OMAP_DEVICE_TYPE_NS = 1,
 OMAP_DEVICE_TYPE_HS = 2,
 OMAP_DEVICE_TYPE_INVALID,
};

#define OMAP_GET_DEVICE_TYPE_IOCTL 0x0001

#endif

