# Copyright 2010 3LM

# First build the proxy daemon (always runs).

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := main.cpp

LOCAL_SHARED_LIBRARIES := libsysutils libcutils libnetutils

LOCAL_MODULE := tund

include $(BUILD_EXECUTABLE)

# Now build the control tool (only for testing).

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= tdc.c

LOCAL_MODULE:= tdc

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)

LOCAL_CFLAGS := 

LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_EXECUTABLE)

