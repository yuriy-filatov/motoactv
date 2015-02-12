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
#ifndef __OMAP_PANEL_H__
#define __OMAP_PANEL_H__

#define OMAP_PANEL_MAX_NAME_SIZE (32)

struct omap_panel_info {

 int idx;

 char name[OMAP_PANEL_MAX_NAME_SIZE + 1];

 int active;
};

struct omap_panel_fod {

 char name[OMAP_PANEL_MAX_NAME_SIZE + 1];

 int enable;
};

#define OMAP_PANEL_IOCTL_MAGIC 'g'
#define OMAP_PANEL_IOCTL_BASE 0x60

#define OMAP_PANEL_QUERYPANEL _IOWR(OMAP_PANEL_IOCTL_MAGIC,   OMAP_PANEL_IOCTL_BASE+0,   struct omap_panel_info)

#define OMAP_PANEL_G_FOD _IOWR(OMAP_PANEL_IOCTL_MAGIC,   OMAP_PANEL_IOCTL_BASE+1,   struct omap_panel_fod)
#define OMAP_PANEL_S_FOD _IOW(OMAP_PANEL_IOCTL_MAGIC,   OMAP_PANEL_IOCTL_BASE+2,   struct omap_panel_fod)

#endif


