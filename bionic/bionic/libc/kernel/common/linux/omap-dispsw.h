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

#ifndef __OMAP_DISPSW_H__
#define __OMAP_DISPSW_H__

#define DISPSW_MAX_NAME_SIZE (32)
#define DISPSW_MAX_PLANES (10)

struct dispsw_info {

 int idx;

 char name[DISPSW_MAX_NAME_SIZE + 1];

 int active;

 int multi_resolutions;

 char resolution_name[DISPSW_MAX_NAME_SIZE + 1];
};

struct dispsw_res_info {

 char name[DISPSW_MAX_NAME_SIZE + 1];

 int idx;

 char resolution_name[DISPSW_MAX_NAME_SIZE + 1];
};

enum dispsw_cmd_type {
 DISPSW_CMD_SET = 1,
 DISPSW_CMD_UNSET,
 DISPSW_CMD_SWITCH_TO,
 DISPSW_CMD_CONFIG,
};

enum dispsw_rotate {
 DISPSW_ROTATE_IGNORE,
 DISPSW_ROTATE_0,
 DISPSW_ROTATE_90,
 DISPSW_ROTATE_180,
 DISPSW_ROTATE_270,
};

enum dispsw_scale {
 DISPSW_SCALE_IGNORE,
 DISPSW_SCALE_FIT_TO_SCREEN,
 DISPSW_SCALE_PERCENT,
};

enum dispsw_align {
 DISPSW_ALIGN_IGNORE,
 DISPSW_ALIGN_CENTER,
 DISPSW_ALIGN_TOP,
 DISPSW_ALIGN_TOP_LEFT,
 DISPSW_ALIGN_TOP_CENTER,
 DISPSW_ALIGN_TOP_RIGHT,
 DISPSW_ALIGN_BOTTOM,
 DISPSW_ALIGN_BOTTOM_LEFT,
 DISPSW_ALIGN_BOTTOM_CENTER,
 DISPSW_ALIGN_BOTTOM_RIGHT,
 DISPSW_ALIGN_LEFT,
 DISPSW_ALIGN_LEFT_CENTER,
 DISPSW_ALIGN_RIGHT,
 DISPSW_ALIGN_RIGHT_CENTER,
 DISPSW_ALIGN_PERCENT,
};

struct dispsw_plane {
 int plane;
 int override;
 int persist;

 int lock_aspect_ratio;
 enum dispsw_rotate rotate;
 enum dispsw_scale scale;
 int v_scale_percent;
 int h_scale_percent;
 enum dispsw_align align;
 int v_align_percent;
 int h_align_percent;
};

struct dispsw_cmd {
 enum dispsw_cmd_type type;
 char name[DISPSW_MAX_NAME_SIZE];

 struct dispsw_plane planes[DISPSW_MAX_PLANES];
};

struct dispsw_res {
 char name[DISPSW_MAX_NAME_SIZE];
 char resolution_name[DISPSW_MAX_NAME_SIZE + 1];
};

#define DISPSW_IOCTL_MAGIC 'g'
#define DISPSW_IOCTL_BASE 0x20
#define DISPSW_QUERYDISP _IOWR(DISPSW_IOCTL_MAGIC,   DISPSW_IOCTL_BASE+0, struct dispsw_info)
#define DISPSW_QUERYRES _IOWR(DISPSW_IOCTL_MAGIC,   DISPSW_IOCTL_BASE+1, struct dispsw_res_info)
#define DISPSW_G_CMD _IOWR(DISPSW_IOCTL_MAGIC,   DISPSW_IOCTL_BASE+2, struct dispsw_cmd)
#define DISPSW_S_CMD _IOW(DISPSW_IOCTL_MAGIC,   DISPSW_IOCTL_BASE+3, struct dispsw_cmd)
#define DISPSW_S_RES _IOW(DISPSW_IOCTL_MAGIC,   DISPSW_IOCTL_BASE+4, struct dispsw_res)

#endif


