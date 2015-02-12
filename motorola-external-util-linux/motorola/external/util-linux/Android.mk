LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

fdisk_files := \
	fdisk_src/fdisk.c \
	fdisk_src/fdiskbsdlabel.c \
	fdisk_src/fdisksgilabel.c \
	fdisk_src/fdisksunlabel.c \
	fdisk_src/fdiskaixlabel.c \
	fdisk_src/fdiskmaclabel.c \
	fdisk_src/partname.c \
	fdisk_src/disksize.c \
	fdisk_src/i386_sys_types.c \
	fdisk_src/gpt.c
fdisk_headers := $(LOCAL_PATH)/include

LOCAL_SRC_FILES:= $(fdisk_files)
LOCAL_C_INCLUDES:= $(fdisk_headers)
LOCAL_CFLAGS += -include $(LOCAL_PATH)/obj/config.h

LOCAL_MODULE:= fdisk

LOCAL_SHARED_LIBRARIES := libc

include $(BUILD_EXECUTABLE)
