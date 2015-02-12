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
enum {
 MASK_BP_READY_AP = 0x0001,
 MASK_BP_READY2_AP = 0x0002,
 MASK_BP_RESOUT = 0x0004,
 MASK_BP_PWRON = 0x0008,
 MASK_AP_TO_BP_PSHOLD = 0x0010,
 MASK_AP_TO_BP_FLASH_EN = 0x0020
};
typedef unsigned short omap_mdm_ctrl_gpiomask;

#define OMAP_MDM_CTRL_IOCTL_GET_BP_READY_AP 0x0001
#define OMAP_MDM_CTRL_IOCTL_GET_BP_READY2_AP 0x0002
#define OMAP_MDM_CTRL_IOCTL_GET_BP_RESOUT 0x0003

#define OMAP_MDM_CTRL_IOCTL_GET_BP_STATUS 0x0004

#define OMAP_MDM_CTRL_IOCTL_SET_BP_PWRON 0x0010
#define OMAP_MDM_CTRL_IOCTL_SET_AP_TO_BP_PSHOLD 0x0020
#define OMAP_MDM_CTRL_IOCTL_SET_AP_TO_BP_FLASH_EN 0x0030

#define OMAP_MDM_CTRL_IOCTL_SET_AP_STATUS 0x0040

#define OMAP_MDM_CTRL_IOCTL_SET_INT_BP_READY_AP 0x0100
#define OMAP_MDM_CTRL_IOCTL_SET_INT_BP_READY2_AP 0x0200
#define OMAP_MDM_CTRL_IOCTL_SET_INT_BP_RESOUT 0x0300

#define OMAP_MDM_CTRL_IOCTL_BP_SHUTDOWN 0x1000
#define OMAP_MDM_CTRL_IOCTL_BP_STARTUP 0x2000
#define OMAP_MDM_CTRL_IOCTL_BP_RESET 0x3000

#define OMAP_MDM_CTRL_IRQ_RISING 0x01
#define OMAP_MDM_CTRL_IRQ_FALLING 0x02

#define OMAP_MDM_CTRL_GPIO_HIGH 1
#define OMAP_MDM_CTRL_GPIO_LOW 0

