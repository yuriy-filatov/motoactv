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
#ifndef _LINUX_MDM_CTRL_H__
#define _LINUX_MDM_CTRL_H__

enum {
 MASK_BP_RESOUT = 0x0001,
};

enum {
 MDM_CTRL_BP_MODE_INVALID = 0,
 MDM_CTRL_BP_MODE_NORMAL,
 MDM_CTRL_BP_MODE_FLASH,
 MDM_CTRL_BP_MODE_UNKNOWN,
};

#define MDM_CTRL_MAGIC 0xEF

#define MDM_CTRL_IOCTL_GET_BP_READY_AP _IOR(MDM_CTRL_MAGIC, 0, int)
#define MDM_CTRL_IOCTL_GET_BP_READY2_AP _IOR(MDM_CTRL_MAGIC, 1, int)
#define MDM_CTRL_IOCTL_GET_BP_RESOUT _IOR(MDM_CTRL_MAGIC, 2, int)
#define MDM_CTRL_IOCTL_GET_BP_STATUS _IOR(MDM_CTRL_MAGIC, 3, int)

#define MDM_CTRL_IOCTL_SET_BP_PWRON _IOW(MDM_CTRL_MAGIC, 100, int)
#define MDM_CTRL_IOCTL_SET_BP_PSHOLD _IOW(MDM_CTRL_MAGIC, 101, int)
#define MDM_CTRL_IOCTL_SET_BP_FLASH_EN _IOW(MDM_CTRL_MAGIC, 102, int)
#define MDM_CTRL_IOCTL_SET_AP_STATUS _IOW(MDM_CTRL_MAGIC, 103, int)
#define MDM_CTRL_IOCTL_SET_BP_RESIN _IOW(MDM_CTRL_MAGIC, 104, int)
#define MDM_CTRL_IOCTL_SET_BP_BYPASS _IOW(MDM_CTRL_MAGIC, 105, int)
#define MDM_CTRL_IOCTL_SET_BP_FLASH_MODE _IOW(MDM_CTRL_MAGIC, 106, int)

#define MDM_CTRL_IOCTL_SET_INT_BP_READY_AP _IOW(MDM_CTRL_MAGIC, 200, int)
#define MDM_CTRL_IOCTL_SET_INT_BP_READY2_AP _IOW(MDM_CTRL_MAGIC, 201, int)
#define MDM_CTRL_IOCTL_SET_INT_BP_RESOUT _IOW(MDM_CTRL_MAGIC, 202, int)
#define MDM_CTRL_IOCTL_SET_INT_BP_STATUS _IOW(MDM_CTRL_MAGIC, 203, int)

#define MDM_CTRL_IOCTL_BP_STARTUP _IOW(MDM_CTRL_MAGIC, 300, int)
#define MDM_CTRL_IOCTL_BP_SHUTDOWN _IO(MDM_CTRL_MAGIC, 301)
#define MDM_CTRL_IOCTL_BP_RESET _IO(MDM_CTRL_MAGIC, 302)

#define MDM_CTRL_IRQ_RISING 0x01
#define MDM_CTRL_IRQ_FALLING 0x02

#define MDM_CTRL_GPIO_HIGH 1
#define MDM_CTRL_GPIO_LOW 0

#define MDM_CTRL_BP_STATUS_UNKNOWN 0x0
#define MDM_CTRL_BP_STATUS_PANIC_WAIT 0x1
#define MDM_CTRL_BP_STATUS_CORE_DUMP 0x2
#define MDM_CTRL_BP_STATUS_RDL 0x3
#define MDM_CTRL_BP_STATUS_AWAKE 0x4
#define MDM_CTRL_BP_STATUS_ASLEEP 0x5
#define MDM_CTRL_BP_STATUS_SHUTDOWN_ACK 0x6
#define MDM_CTRL_BP_STATUS_PANIC 0x7

#define MDM_CTRL_AP_STATUS_UNKNOWN 0x0
#define MDM_CTRL_AP_STATUS_DATA_BYPASS 0x1
#define MDM_CTRL_AP_STATUS_FULL_BYPASS 0x2
#define MDM_CTRL_AP_STATUS_NO_BYPASS 0x3
#define MDM_CTRL_AP_STATUS_BP_SHUTDOWN 0x4
#define MDM_CTRL_AP_STATUS_FLASH_MODE 0x6
#define MDM_CTRL_AP_STATUS_BP_PANIC_ACK 0x7

#define MDM_CTRL_BP_FLASH_MODE_NONE 0x0
#define MDM_CTRL_BP_FLASH_MODE_RSD 0x1
#define MDM_CTRL_BP_FLASH_MODE_QCOM 0x2

#endif

