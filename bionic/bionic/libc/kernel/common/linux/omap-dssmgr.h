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
#include <linux/ioctl.h>

#ifndef __OMAP_DSSMGR_H__
#define __OMAP_DSSMGR_H__

#define DSSMGR_MAX_NAME_SIZE (32)

#define DSSMGR_MAX_DISPLAYS (5)
#define DSSMGR_MAX_MANAGERS (5)
#define DSSMGR_MAX_OVERLAYS (10)

#define DSSMGR_EDID_BLOCK_LEN (128)

#define DSSMGR_MAX_RESOLUTIONS (30)

#define DSSMGR_DPY_FLAG_ACTIVE (1 << 0)
#define DSSMGR_DPY_FLAG_HOTPLUG (1 << 1)
#define DSSMGR_DPY_FLAG_EDID (1 << 2)

#define DSSMGR_RES_FLAG_HDMI (1 << 0)
#define DSSMGR_RES_FLAG_INTERLACED (1 << 1)

struct dssmgr_resolution {
 uint16_t width;
 uint16_t height;
 uint16_t rate;
 uint16_t flags;
};

struct dssmgr_display {

 int id;

 char name[DSSMGR_MAX_NAME_SIZE + 1];

 uint32_t flags;

 struct dssmgr_resolution resolution;
};

struct dssmgr_manager {

 int id;

 char name[DSSMGR_MAX_NAME_SIZE + 1];

 int disp_id;
};

struct dssmgr_overlay {

 int id;

 char name[DSSMGR_MAX_NAME_SIZE + 1];

 int mgr_id;

 int active;
};

struct dssmgr_cfg {
 struct dssmgr_display displays[DSSMGR_MAX_DISPLAYS];
 struct dssmgr_manager managers[DSSMGR_MAX_MANAGERS];
 struct dssmgr_overlay overlays[DSSMGR_MAX_OVERLAYS];
};

enum dssmgr_rotation {

 DSSMGR_ROTATION_IGNORE,
 DSSMGR_ROTATION_0,
 DSSMGR_ROTATION_90,
 DSSMGR_ROTATION_180,
 DSSMGR_ROTATION_270,
};

enum dssmgr_scale {

 DSSMGR_SCALE_IGNORE,

 DSSMGR_SCALE_FILL_SCREEN,

 DSSMGR_SCALE_FIT_TO_SCREEN,
};

enum dssmgr_cmd_type {

 DSSMGR_CMD_ENABLE_DPY = 1,

 DSSMGR_CMD_ATTACH_OLY2MGR,

 DSSMGR_CMD_ATTACH_MGR2DPY,

 DSSMGR_CMD_SET_RESOLUTION,

 DSSMGR_CMD_ENABLE_HPD,

 DSSMGR_CMD_RESET_OLY,

 DSSMGR_CMD_FORCE_DISABLE_OLY,

 DSSMGR_CMD_FORCE_ROTATION_OLY,

 DSSMGR_CMD_FORCE_SCALE_OLY,
};

struct dssmgr_cmd {

 enum dssmgr_cmd_type cmd;

 int comp_id1;
 int comp_id2;

 uint32_t val1;

 struct dssmgr_resolution resolution;
};

struct dssmgr_edid {

 int dpy_id;

 int block;

 uint8_t data[DSSMGR_EDID_BLOCK_LEN];
};

#define DSSMGR_IOCTL_MAGIC 'g'
#define DSSMGR_IOCTL_BASE 0x20
#define DSSMGR_QUERYCFG _IOWR(DSSMGR_IOCTL_MAGIC,   DSSMGR_IOCTL_BASE+0, struct dssmgr_cfg)
#define DSSMGR_S_CMD _IOW(DSSMGR_IOCTL_MAGIC,   DSSMGR_IOCTL_BASE+1, struct dssmgr_cmd)
#define DSSMGR_G_EDID _IOW(DSSMGR_IOCTL_MAGIC,   DSSMGR_IOCTL_BASE+2, struct dssmgr_edid)

#endif


