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
#ifndef _LINUX_ISL29030_H__
#define _LINUX_ISL29030_H__

#include <linux/ioctl.h>

#define ISL29030_IOCTL_BASE 0xA3
#define ISL29030_IOCTL_GET_ENABLE _IOR(ISL29030_IOCTL_BASE, 0x00, char)
#define ISL29030_IOCTL_SET_ENABLE _IOW(ISL29030_IOCTL_BASE, 0x01, char)
#define ISL29030_IOCTL_GET_LIGHT_ENABLE _IOR(ISL29030_IOCTL_BASE, 0x02, char)
#define ISL29030_IOCTL_SET_LIGHT_ENABLE _IOW(ISL29030_IOCTL_BASE, 0x03, char)

#endif

