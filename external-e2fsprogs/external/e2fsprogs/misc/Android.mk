LOCAL_PATH := $(call my-dir)

#########################################################################
# Build mke2fs
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mke2fs.c \
	util.c \
	default_profile.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib \
	external/e2fsprogs/e2fsck

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

LOCAL_CFLAGS += -DNO_CHECK_BB

LOCAL_MODULE := mke2fs

LOCAL_SYSTEM_SHARED_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
	libext2_e2p \
	libc

include $(BUILD_EXECUTABLE)

# build static executable
SAVED_SRC_FILES := $(LOCAL_SRC_FILES)
SAVED_SYSTEM_LIBRARIES := $(LOCAL_SYSTEM_SHARED_LIBRARIES)
SAVED_C_INCLUDES := $(LOCAL_C_INCLUDES)
SAVED_CFLAGS := $(LOCAL_CFLAGS)
include $(CLEAR_VARS)
LOCAL_MODULE := mke2fs_static
LOCAL_SRC_FILES := $(SAVED_SRC_FILES)
LOCAL_STATIC_LIBRARIES := $(SAVED_SYSTEM_LIBRARIES)
LOCAL_C_INCLUDES := $(SAVED_C_INCLUDES)
LOCAL_CFLAGS := $(SAVED_CFLAGS)
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

###########################################################################
# Build tune2fs
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	tune2fs.c \
	util.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib \
	external/e2fsprogs/e2fsck

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

LOCAL_CFLAGS += -DNO_CHECK_BB

LOCAL_MODULE := tune2fs
LOCAL_MODULE_TAGS := eng user
LOCAL_SYSTEM_SHARED_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_com_err \
	libext2_uuid \
	libext2_e2p \
	libc

include $(BUILD_EXECUTABLE)

#########################################################################
# Build badblocks
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	badblocks.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

LOCAL_MODULE := badblocks
LOCAL_MODULE_TAGS := systembuilder

LOCAL_SYSTEM_SHARED_LIBRARIES := \
	libext2fs \
	libext2_com_err \
	libc

include $(BUILD_EXECUTABLE)

#########################################################################
# Build dumpe2fs
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	dumpe2fs.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib \
	external/e2fsprogs/e2fsck

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

LOCAL_CFLAGS += -DNO_CHECK_BB

LOCAL_MODULE := dumpe2fs
LOCAL_MODULE_TAGS := eng user

LOCAL_SYSTEM_SHARED_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
	libext2_e2p \
	libc

include $(BUILD_EXECUTABLE)

###############################################################
# HOST BUILD
###############################################################
# Build mke2fs
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mke2fs.c \
	util.c \
	default_profile.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib \
	external/e2fsprogs/e2fsck

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

ifneq ($(HOST_OS),darwin)
LOCAL_CFLAGS += -DHAVE_SYS_PRCTL_H \
        -DHAVE_LINUX_FD_H \
        -DHAVE_LSEEK64 \
        -DHAVE_LSEEK64_PROTOTYPE
else
LOCAL_CFLAGS += -DHAVE_LSEEK \
        -DHAVE_LSEEK_PROTOTYPE
endif

LOCAL_CFLAGS += -DNO_CHECK_BB

LOCAL_MODULE := mke2fs

LOCAL_STATIC_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
	libext2_e2p

include $(BUILD_HOST_EXECUTABLE)

###########################################################################
# Build tune2fs
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	tune2fs.c \
	util.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib \
	external/e2fsprogs/e2fsck

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

ifneq ($(HOST_OS),darwin)
LOCAL_CFLAGS += -DHAVE_SYS_PRCTL_H \
        -DHAVE_LINUX_FD_H \
        -DHAVE_LSEEK64 \
        -DHAVE_LSEEK64_PROTOTYPE
else
LOCAL_CFLAGS += -DHAVE_LSEEK \
        -DHAVE_LSEEK_PROTOTYPE
endif

LOCAL_CFLAGS += -DNO_CHECK_BB

LOCAL_MODULE := tune2fs
LOCAL_MODULE_TAGS := eng user
LOCAL_STATIC_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
	libext2_e2p

include $(BUILD_HOST_EXECUTABLE)

#########################################################################
# Build badblocks
#
ifneq ($(HOST_OS),darwin)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	badblocks.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

LOCAL_MODULE := badblocks
LOCAL_MODULE_TAGS := systembuilder

LOCAL_STATIC_LIBRARIES := \
	libext2fs \
	libext2_com_err

include $(BUILD_HOST_EXECUTABLE)

endif
#########################################################################
# Build dumpe2fs
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	dumpe2fs.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib \
	external/e2fsprogs/e2fsck

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

ifneq ($(HOST_OS),darwin)
LOCAL_CFLAGS += -DHAVE_SYS_PRCTL_H \
        -DHAVE_LINUX_FD_H \
        -DHAVE_LSEEK64 \
        -DHAVE_LSEEK64_PROTOTYPE
else
LOCAL_CFLAGS += -DHAVE_LSEEK \
        -DHAVE_LSEEK_PROTOTYPE
endif

LOCAL_CFLAGS += -DNO_CHECK_BB

LOCAL_MODULE := dumpe2fs
LOCAL_MODULE_TAGS := eng user

LOCAL_STATIC_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
	libext2_e2p

include $(BUILD_HOST_EXECUTABLE)
