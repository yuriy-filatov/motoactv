LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        btio.c \
        oui.c \
        sdp-xml.c \
        textfile.c \
        android_bluez.c

LOCAL_CFLAGS+= \
        -O3 \
        -DNEED_DBUS_WATCH_GET_UNIX_FD

ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
LOCAL_CFLAGS += \
        -DBOARD_HAVE_BLUETOOTH_BCM
endif

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/../lib \
        $(LOCAL_PATH)/../src \
        $(call include-path-for, glib) \
        $(call include-path-for, glib)/glib \
        $(call include-path-for, dbus)

LOCAL_STATIC_LIBRARY:= \
        libgatt_static \
        libglib_static

LOCAL_MODULE_TAGS := eng debug

LOCAL_MODULE:=libbluez-common-static

include $(BUILD_STATIC_LIBRARY)

