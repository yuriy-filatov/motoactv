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
#ifndef __TCMD_H__
#define __TCMD_H__

#define TCMD_IOCTL_BASE 0x0a
#define TCMD_IOCTL_MASK_INT _IOW(TCMD_IOCTL_BASE, 0x01, int)
#define TCMD_IOCTL_UNMASK_INT _IOW(TCMD_IOCTL_BASE, 0x02, int)
#define TCMD_IOCTL_READ_INT _IOWR(TCMD_IOCTL_BASE, 0x03, int)
#define TCMD_IOCTL_SET_INT _IOW(TCMD_IOCTL_BASE, 0x04, char)

enum tcmd_gpio_enum {
 TCMD_GPIO_ISL29030_INT = 0,
 TCMD_GPIO_KXTF9_INT,
 TCMD_GPIO_MMC_DETECT,
 TCMD_GPIO_WC_STAT_0,
 TCMD_GPIO_WC_STAT_1,
 TCMD_GPIO_WC_CTRL_0,
 TCMD_GPIO_WC_CTRL_1,
 TCMD_GPIO_WC_CHG_COMPLETE,
 TCMD_GPIO_WC_CHG_TERMINATE,
 TCMD_GPIO_LTE_WAN_HOSTWAKE,
 TCMD_GPIO_LTE_FORCE_FLASH,
 TCMD_GPIO_LTE_W_DISABLE_B,
 TCMD_GPIO_LTE_WAN_USB_ENABLE,
 TCMD_GPIO_PROX_SFH7743,
 TCMD_GPIO_FACTORY_KILL_OVERRIDE,
 TCMD_GPIO_INT_MAX_NUM
};

struct tcmd_gpio_set_arg {
 unsigned char gpio;
 unsigned char gpio_state;
};

struct tcmd_gpio_mapping {
 enum tcmd_gpio_enum gpio_index;
 char                gpio_name[40];
};

struct tcmd_driver_platform_data {
 int size;
 int gpio_list[TCMD_GPIO_INT_MAX_NUM+1];
};

#endif

