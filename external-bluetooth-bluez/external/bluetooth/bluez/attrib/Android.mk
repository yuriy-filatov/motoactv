LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
         gattrib.c \
         gatt.c \
         att.c

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/../lib \
        $(LOCAL_PATH)/../plugins \
        $(LOCAL_PATH)/../common \
        $(LOCAL_PATH)/../src \
        $(LOCAL_PATH)/../gdbus \
        $(LOCAL_PATH)/../../glib/glib \
        $(call include-path-for, glib) \
        $(call include-path-for, glib)/glib \
        $(call include-path-for, dbus)

LOCAL_SHARED_LIBRARY:= libcutils

LOCAL_MODULE_TAGS := eng 

LOCAL_STATIC_LIBRARY:= \
        libbluez-common-static \
        libglib_static

LOCAL_MODULE:=libgatt_static

include $(BUILD_STATIC_LIBRARY)




include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	gatttool.c \
	utils.c

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH)/../lib \
        $(LOCAL_PATH)/../plugins \
        $(LOCAL_PATH)/../common \
        $(LOCAL_PATH)/../src \
        $(LOCAL_PATH)/../gdbus \
        $(LOCAL_PATH)/../../glib/glib \
        $(call include-path-for, glib) \
        $(call include-path-for, glib)/glib \
        $(call include-path-for, dbus)

LOCAL_SHARED_LIBRARIES := \
        libbluetooth \
        libbluetoothd \
        libdbus

LOCAL_STATIC_LIBRARIES := \
       libgatt_static \
       libgdbus_static \
       libbuiltinplugin \
       libglib_static

LOCAL_MODULE_TAGS:=eng

LOCAL_MODULE:=gatttool

include $(BUILD_EXECUTABLE)




