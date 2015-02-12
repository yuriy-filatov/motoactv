LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

PROJECTM_ROOT := $(LOCAL_PATH)

# core
PROJECTM_SRC := ./
CONFIG_DIRNAME := ./ 

# C/C++ compiling flags
LOCAL_CFLAGS += -fPIC -w 
#LOCAL_CFLAGS += -O1 -DVALIDATE_DEF_REF 
LOCAL_CFLAGS += -fno-inline 
LOCAL_CPPFLAGS += -fno-exceptions -fno-rtti -DXTHREADS -D_REENTRANT -DXUSE_MTSAFE_API
LOCAL_CPPFLAGS += -Os -pipe -Wall -fPIC -fcheck-new -fno-inline -fno-exceptions -fno-rtti

# Build flashlite (core) library
include $(LOCAL_PATH)/ProjectM.mk

