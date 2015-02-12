LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
ip6tunnel.c    ip.c          ipmroute.c    iptunnel.c  link_veth.c   \
ipaddress.c    iplink.c      ipmaddr.c      ipneigh.c   iproute.c    rtm_map.c    tunnel.c         \
ipaddrlabel.c  iplink_can.c  ipmonitor.c    ipntable.c  iprule.c    link_gre.c

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := ip

LOCAL_SYSTEM_SHARED_LIBRARIES := \
    libc libm libdl

LOCAL_SHARED_LIBRARIES += libiprouteutil libnetlink

LOCAL_C_INCLUDES := $(KERNEL_HEADERS) external/iproute2/include

LOCAL_CFLAGS := -O2 -g -W -Wall

include $(BUILD_EXECUTABLE)

