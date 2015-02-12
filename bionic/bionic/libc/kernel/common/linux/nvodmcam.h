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
#ifndef __LINUX_NVODMCAM_H__
#define __LINUX_NVODMCAM_H__

#define NVODMCAM_MAGIC 0xF1

#define NVODMCAM_IOCTL_GET_NUM_CAM _IOR(NVODMCAM_MAGIC, 100, int)

#define NVODMCAM_IOCTL_CAMERA_OFF _IO(NVODMCAM_MAGIC, 200)
#define NVODMCAM_IOCTL_CAMERA_ON _IOW(NVODMCAM_MAGIC, 201, int)

#endif

