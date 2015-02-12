ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := dumpext3fs.c
LOCAL_MODULE := dumpext3fs
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_C_INCLUDES += external/e2fsprogs/lib
LOCAL_STATIC_LIBRARIES += libext2fs libext2_com_err libext2_e2p

include $(BUILD_HOST_EXECUTABLE)

endif  # !TARGET_SIMULATOR
