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
#ifndef __LIS3DH_H__
#define __LIS3DH_H__

#include <linux/ioctl.h>

#define LIS3DH_IOCTL_BASE 71

#define LIS3DH_IOCTL_SET_DELAY _IOW(LIS3DH_IOCTL_BASE, 0, int)
#define LIS3DH_IOCTL_GET_DELAY _IOR(LIS3DH_IOCTL_BASE, 1, int)
#define LIS3DH_IOCTL_SET_ENABLE _IOW(LIS3DH_IOCTL_BASE, 2, int)
#define LIS3DH_IOCTL_GET_ENABLE _IOR(LIS3DH_IOCTL_BASE, 3, int)

#endif


