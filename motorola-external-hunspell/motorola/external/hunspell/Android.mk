LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

# LOCAL_ARM_MODE := arm
LOCAL_CPP_EXTENSION := .cxx 

LOCAL_SRC_FILES:= \
	hunspell/affentry.cxx	hunspell/dictmgr.cxx	\
	hunspell/suggestmgr.cxx	hunspell/affixmgr.cxx	\
	hunspell/filemgr.cxx	hunspell/utf_info.cxx	\
	hunspell/hunzip.cxx	hunspell/phonet.cxx	\
	hunspell/hashmgr.cxx	hunspell/csutil.cxx	\
	hunspell/replist.cxx	hunspell/hunspell.cxx	\
    tools/jhunCheck.cxx


LOCAL_C_INCLUDES =       \
    $(LOCAL_PATH)/hunspell	\
	$(LOCAL_PATH)/tools	\
	$(JNI_H_INCLUDE)

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -DARM_FLAG
endif

LOCAL_CFLAGS += \
        -DJNI_EXPORTS \
        
LOCAL_CFLAGS +=  -D_REENTRANT -DPIC -DU_COMMON_IMPLEMENTATION -fPIC -DLINUX -DHAVE_CONFIG_H

LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_LDLIBS           += -lm -lstdc++ -lpthread -ldl

LOCAL_MODULE := libspellingcheckengine

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
