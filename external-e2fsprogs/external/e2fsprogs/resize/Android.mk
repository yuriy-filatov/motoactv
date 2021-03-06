LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	extent.c \
	resize2fs.c \
	main.c \
	online.c \
	sim_progress.c

LOCAL_C_INCLUDES := external/e2fsprogs/lib

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
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T


LOCAL_MODULE := resize2fs
LOCAL_SYSTEM_SHARED_LIBRARIES := libext2fs libext2_com_err libext2_e2p libc


include $(BUILD_EXECUTABLE)

###############################################################
# HOST BUILD
###############################################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	extent.c \
	resize2fs.c \
	main.c \
	online.c \
	sim_progress.c

LOCAL_C_INCLUDES := external/e2fsprogs/lib

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
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_TYPE_SSIZE_T

ifneq ($(HOST_OS),darwin)
LOCAL_CFLAGS += -DHAVE_SYS_PRCTL_H \
        -DHAVE_LINUX_FD_H \
        -DHAVE_LSEEK64 \
        -DHAVE_LSEEK64_PROTOTYPE
else
LOCAL_CFLAGS += -DHAVE_LSEEK \
        -DHAVE_LSEEK_PROTOTYPE
endif

LOCAL_MODULE := resize2fs
LOCAL_STATIC_LIBRARIES := libext2fs libext2_com_err libext2_e2p


include $(BUILD_HOST_EXECUTABLE)
