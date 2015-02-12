# Copyright 2006 The Android Open Source Project

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := tuncfg.c

LOCAL_SHARED_LIBRARIES := libnetutils

LOCAL_MODULE := tuncfg

include $(BUILD_EXECUTABLE)
