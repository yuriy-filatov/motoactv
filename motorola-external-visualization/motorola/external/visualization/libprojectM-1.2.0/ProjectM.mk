LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)

# BEGIN MOT FROYO UPMERGE, a18772, 05/24/2010
# Change to use external/stlport introduced in Froyo
# instead of original motorola/external/stlport

include $(BUILD_MULTI_PREBUILT)
# Define our module 
LOCAL_PATH := $(MY_LOCAL_PATH)

#------------------------
#LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libprojectM
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

libprojectM_includes:= \
	bionic \
	external/stlport/stlport

# Build our list of include paths
LOCAL_C_INCLUDES := \
	$(libprojectM_includes) \
	$(PROJECTM_ROOT)/ \
	frameworks/base/opengl/include \
	frameworks/base/opengl/include/GLES \
	frameworks/base/include

# END MOTO FROYO UPMERGE, a18772, 05/24/2010

# Build our list of source paths
# General src files
LOCAL_SRC_FILES:= \
       ${PROJECTM_SRC}/BeatDetect.cpp \
       ${PROJECTM_SRC}/BuiltinFuncs.cpp \
       ${PROJECTM_SRC}/BuiltinParams.cpp \
       ${PROJECTM_SRC}/ConfigFile.cpp \
       ${PROJECTM_SRC}/CustomShape.cpp \
       ${PROJECTM_SRC}/CustomWave.cpp \
       ${PROJECTM_SRC}/Eval.cpp \
       ${PROJECTM_SRC}/Expr.cpp \
       ${PROJECTM_SRC}/FBO.cpp \
       ${PROJECTM_SRC}/fftsg.cpp \
       ${PROJECTM_SRC}/Func.cpp \
       ${PROJECTM_SRC}/IdlePreset.cpp \
       ${PROJECTM_SRC}/InitCond.cpp \
       ${PROJECTM_SRC}/KeyHandler.cpp \
       ${PROJECTM_SRC}/Param.cpp \
       ${PROJECTM_SRC}/Parser.cpp \
       ${PROJECTM_SRC}/PCM.cpp \
       ${PROJECTM_SRC}/PerFrameEqn.cpp \
       ${PROJECTM_SRC}/PerPixelEqn.cpp \
       ${PROJECTM_SRC}/PerPointEqn.cpp \
       ${PROJECTM_SRC}/PresetChooser.cpp \
       ${PROJECTM_SRC}/Preset.cpp \
       ${PROJECTM_SRC}/PresetFrameIO.cpp \
       ${PROJECTM_SRC}/PresetLoader.cpp \
       ${PROJECTM_SRC}/PresetMerge.cpp \
       ${PROJECTM_SRC}/projectM.cpp \
       ${PROJECTM_SRC}/Renderer.cpp \
       ${PROJECTM_SRC}/TextureManager.cpp \
       ${PROJECTM_SRC}/TimeKeeper.cpp \
       ${PROJECTM_SRC}/timer.cpp \
       ${PROJECTM_SRC}/wipemalloc.cpp \

#SOIL sources
LOCAL_SRC_FILES += \
       ${PROJECTM_SRC}/SOIL.c \
       ${PROJECTM_SRC}/image_DXT.c \
       ${PROJECTM_SRC}/image_helper.c \
       ${PROJECTM_SRC}/stb_image_aug.c \

#GLEW files
#LOCAL_SRC_FILES += \
       ${PROJECTM_SRC}/glew.c \

# Define our compiler flags
LOCAL_CFLAGS := \
        -DUSE_GLES1 \
        -DUSE_NATIVE_GLEW \
        -DPROJECTM_ANDROID \
        -DPROJECTM_ANDROID_DISABLE_EXCEPTION \
        -D_STLP_REAL_LOCALE_IMPLEMENTED \
        -DLINUX \
        -D__MARM__  \
        -D__ARM9E__ \
        -D__ARM__

#LOCAL_CFLAGS += -DAMP_GENERIC_ARM_ENV
#LOCAL_CFLAGS += -D__GCCE__ -D__arm__ -DAMP_ARCH_ARM
	
#LOCAL_CFLAGS += -DDEBUG -D_DEBUG	
LOCAL_CFLAGS   += -fPIC  
#LOCAL_CFLAGS   += -fexceptions 

#LOCAL_CFLAGS += -Wno-endif-labels -Wno-import -Wno-format
#LOCAL_CFLAGS += -fno-strict-aliasing
#LOCAL_CFLAGS += -fpic-strict-aliasing
#DEBUG_CFLAGS     := -DDEBUG -D_DEBUG -fPIC -w -DVALIDATE_DEF_REF
#RELEASE_CFLAGS   := -fPIC -w
#DEBUG_CXXFLAGS   := ${DEBUG_CFLAGS} -fno-exceptions -fno-rtti -DXTHREADS -D_REENTRANT -DXUSE_MTSAFE_API
#RELEASE_CXXFLAGS := ${RELEASE_CFLAGS} -fno-exceptions -fno-rtti
#DEBUG_LDFLAGS    := -w
LIBDEBUG_LDFLAGS    := -g -static

#ifeq ($(TARGET_ARCH),arm)
#LOCAL_CFLAGS += -Darm -fvisibility=hidden
#endif

LOCAL_LDLIBS += -lpthread -ldl 

# BEGIN MOTO FROYO UPMERGE, a18772, 05/24/2010
# Change to use libstlport.so shared library

# Build the list of shared libraries
LOCAL_SHARED_LIBRARIES := \
        libutils \
        libstdc++ \
        libEGL \
        libGLESv1_CM \
	libstlport

# END MOTO FROYO UPMERGE, a18772, 05/24/2010
      
# We have to use the android version of libdl when we are not on the simulator
#ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
#endif

# Build the library all at once
include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)



#include $(CLEAR_VARS)
#LOCAL_C_INCLUDES := \
	$(libprojectM_includes) \
	$(PROJECTM_ROOT)/ \
	frameworks/base/opengl/include \
	frameworks/base/include 

#LOCAL_PRELINK_MODULE    := false
#LOCAL_MODULE            := projectM-test
#LOCAL_CFLAGS            := $(COMMON_CFLAGS)
#LOCAL_SRC_FILES         := test/testprojectM.cpp
#LOCAL_SHARED_LIBRARIES  := libEGL \
                           libGLESv1_CM \
                           libprojectMSTL \

#LOCAL_STATIC_LIBRARIES := \
                           libprojectMSTL \

#include $(BUILD_EXECUTABLE)

