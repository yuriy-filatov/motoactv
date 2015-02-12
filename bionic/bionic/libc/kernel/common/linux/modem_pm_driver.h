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
#ifndef __MODEM_PM_DRIVER_H__
#define __MODEM_PM_DRIVER_H__

#include <linux/ioctl.h>

#define MODEM_PM_DRIVER_DEV_NAME "modem_pm_driver"

enum MODEM_PM_SHARED_DDR_FREQUENCY_OPP_CONSTRAINT_T {
 MODEM_PM_SHARED_DDR_FREQUENCY_OPP_HIGH,
 MODEM_PM_SHARED_DDR_FREQUENCY_OPP_NO_VOTE,
};

enum MODEM_PM_SHARED_DDR_LOW_POWER_POLICY_CONSTRAINT_T {
 MODEM_PM_SHARED_DDR_LOW_POWER_POLICY_ON_INACTIVE,
 MODEM_PM_SHARED_DDR_LOW_POWER_POLICY_RET,
 MODEM_PM_SHARED_DDR_LOW_POWER_POLICY_NO_VOTE,
};

#define MODEM_PM_DRIVER_IOCTL_CMD_HANDLE_FREQUENCY_OPP_CONSTRAINT (0x00)

#define MODEM_PM_DRIVER_IOCTL_CMD_HANDLE_LOW_POWER_POLICY_CONSTRAINT (0x01)

#define MODEM_PM_DRIVER_IOCTL_HANDLE_FREQUENCY_OPP_CONSTRAINT   _IOW(0, MODEM_PM_DRIVER_IOCTL_CMD_HANDLE_FREQUENCY_OPP_CONSTRAINT,   enum MODEM_PM_SHARED_DDR_FREQUENCY_OPP_CONSTRAINT_T)

#define MODEM_PM_DRIVER_IOCTL_HANDLE_LOW_POWER_POLICY_CONSTRAINT   _IOW(0, MODEM_PM_DRIVER_IOCTL_CMD_HANDLE_LOW_POWER_POLICY_CONSTRAINT,   enum MODEM_PM_SHARED_DDR_LOW_POWER_POLICY_CONSTRAINT_T)

#endif


