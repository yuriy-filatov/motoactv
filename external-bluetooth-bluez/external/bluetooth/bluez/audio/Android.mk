LOCAL_PATH:= $(call my-dir)

# A2DP plugin

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	a2dp.c \
	avdtp.c \
	control.c \
	device.c \
	gateway.c \
	headset.c \
	ipc.c \
	main.c \
	manager.c \
	module-bluetooth-sink.c \
	sink.c \
	source.c \
	telephony-dummy.c \
	unix.c

LOCAL_CFLAGS:= \
	-DVERSION=\"4.69\" \
	-DSTORAGEDIR=\"/data/misc/bluetoothd\" \
	-DCONFIGDIR=\"/etc/bluetooth\" \
	-DANDROID \
#	-DENABLE_MP3_ITTIAM_CODEC \
	-DADD_RTP_PAYLOAD_HEADER\
#	-DENABLE_CSR_APTX_CODEC \
	-D__S_IFREG=0100000  # missing from bionic stat.h

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../gdbus \
	$(LOCAL_PATH)/../src \
	$(call include-path-for, glib) \
	$(call include-path-for, dbus)

LOCAL_SHARED_LIBRARIES := \
	libbluetooth \
	libbluetoothd \
	libdbus


LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/bluez-plugin
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/bluez-plugin
LOCAL_MODULE := audio

include $(BUILD_SHARED_LIBRARY)

#
# liba2dp
# This is linked to Audioflinger so **LGPL only**

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	liba2dp.c \
	ipc.c \
	mpeg12-payload.c \
	../sbc/sbc.c.arm \
	../sbc/sbc_primitives.c \
	../sbc/sbc_primitives_neon.c

# to improve SBC performance
LOCAL_CFLAGS:= -funroll-loops

LOCAL_CFLAGS += \
#	-DENABLE_MP3_ITTIAM_CODEC \
	-DADD_RTP_PAYLOAD_HEADER 
#	-DENABLE_CSR_APTX_CODEC

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../sbc \
    ../../../../frameworks/base/include \
	system/bluetooth/bluez-clean-headers

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libbt-aptx \
	libbt-mp3 \
	libbt-mpeg12pl

LOCAL_MODULE := liba2dp

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
