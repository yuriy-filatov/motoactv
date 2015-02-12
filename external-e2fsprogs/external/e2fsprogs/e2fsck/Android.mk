LOCAL_PATH := $(call my-dir)

#########################
# Build the libext2 profile library

include $(CLEAR_VARS)
LOCAL_SRC_FILES :=  \
	prof_err.c \
	profile.c

LOCAL_MODULE := libext2_profile
LOCAL_SYSTEM_SHARED_LIBRARIES := \
	libext2_com_err \
	libc

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

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

# build static library
SAVED_SRC_FILES := $(LOCAL_SRC_FILES)
SAVED_MODULE := $(LOCAL_MODULE)
SAVED_SYSTEM_LIBRARIES := $(LOCAL_SYSTEM_SHARED_LIBRARIES)
SAVED_C_INCLUDES := $(LOCAL_C_INCLUDES)
SAVED_CFLAGS := $(LOCAL_CFLAGS)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(SAVED_SRC_FILES)
LOCAL_MODULE := $(SAVED_MODULE)
LOCAL_STATIC_LIBRARIES := $(SAVED_SYSTEM_LIBRARIES)
LOCAL_C_INCLUDES := $(SAVED_C_INCLUDES)
LOCAL_CFLAGS := $(SAVED_CFLAGS)
include $(BUILD_STATIC_LIBRARY)

#########################
# Build the e2fsck binary

include $(CLEAR_VARS)
LOCAL_SRC_FILES :=  \
	e2fsck.c \
	dict.c \
	super.c \
	pass1.c \
	pass1b.c \
	pass2.c \
	pass3.c \
	pass4.c \
	pass5.c \
	journal.c \
	recovery.c \
	revoke.c \
	badblocks.c \
	util.c \
	unix.c \
	dirinfo.c \
	dx_dirinfo.c \
	ehandler.c \
	problem.c \
	message.c \
	swapfs.c \
	ea_refcount.c \
	rehash.c \
	region.c

LOCAL_MODULE := e2fsck

LOCAL_SYSTEM_SHARED_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
	libext2_e2p \
	libc

LOCAL_C_INCLUDES := external/e2fsprogs/lib

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_DIRENT_H \
	-DHAVE_ERRNO_H \
	-DHAVE_INTTYPES_H \
	-DHAVE_LINUX_FD_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SETJMP_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_UNISTD_H \
	-DHAVE_UTIME_H \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_INTPTR_T \
	-DENABLE_HTREE=1

include $(BUILD_EXECUTABLE)

###############################################################
# HOST BUILD
###############################################################
# Build the libext2 profile library

include $(CLEAR_VARS)
LOCAL_SRC_FILES :=  \
	prof_err.c \
	profile.c

LOCAL_MODULE := libext2_profile
LOCAL_STATIC_LIBRARIES := \
	libext2_com_err

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

LOCAL_PRELINK_MODULE := false

include $(BUILD_HOST_STATIC_LIBRARY)


#########################
# Build the e2fsck binary

ifneq ($(HOST_OS),darwin)
include $(CLEAR_VARS)
LOCAL_SRC_FILES :=  \
	e2fsck.c \
	dict.c \
	super.c \
	pass1.c \
	pass1b.c \
	pass2.c \
	pass3.c \
	pass4.c \
	pass5.c \
	journal.c \
	recovery.c \
	revoke.c \
	badblocks.c \
	util.c \
	unix.c \
	dirinfo.c \
	dx_dirinfo.c \
	ehandler.c \
	problem.c \
	message.c \
	swapfs.c \
	ea_refcount.c \
	rehash.c \
	region.c

LOCAL_MODULE := e2fsck

LOCAL_STATIC_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
	libext2_e2p

LOCAL_C_INCLUDES := external/e2fsprogs/lib

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_DIRENT_H \
	-DHAVE_ERRNO_H \
	-DHAVE_INTTYPES_H \
	-DHAVE_LINUX_FD_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SETJMP_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_UNISTD_H \
	-DHAVE_UTIME_H \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_INTPTR_T \
	-DENABLE_HTREE=1

include $(BUILD_HOST_EXECUTABLE)

else
#########################
# IKMAIN-1686 Use compiled binary of e2fsck for MacOS v.1.41.12
#   It should be removed once Google updates e2fsprogs to recent version
#
TARGET_PREBUILT_E2FSCK := $(LOCAL_PATH)/bin/darwin/e2fsck

.PHONY: $(TARGET_PREBUILT_E2FSCK)

$(TARGET_PREBUILT_E2FSCK):
	@echo "Copying e2fsck prebuilt binary for MacOS: $@..."

$(E2FSCK): $(TARGET_PREBUILT_E2FSCK) | $(ACP)
	$(transform-prebuilt-to-target)
endif
