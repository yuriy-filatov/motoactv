LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_MODULE := jcifs-krb5-1.3.12

include $(BUILD_JAVA_LIBRARY)

# ======================================
# Install the permissions file into system/etc/permissions
# ======================================
include $(CLEAR_VARS)
LOCAL_MODULE := jcifs-krb5-1.3.12.xml
LOCAL_MODULE_TAGS := user eng development
LOCAL_MODULE_CLASS := ETC

LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)
