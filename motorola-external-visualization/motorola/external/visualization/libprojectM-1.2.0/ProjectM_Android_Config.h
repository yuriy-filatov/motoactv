
#ifndef _PROJECTM_ANDROID_CONFIG_H
#define _PROJECTM_ANDROID_CONFIG_H

#define LOG_TAG "ProjectM"
#include <utils/Log.h>

//#define VIS_TESTAPP
#define VIS_DUMP_PCM 0
#define VIS_AUTO_SWITCHTORANDOM_0SEC 0
//#define VIS_USE_SINGLEBUFFER
//#define PROJECTM_ENABLE_GLCHECK 

#define PROJECTM_LOGLEVEL 2
//#define ENABLE_PRINTF 
#if  0
#define CUSTOM_WAVE_DEBUG 1
#define EVAL_DEBUG 2
#define EVAL_DEBUG_DOUBLE 2
#define PARAM_DEBUG 1
#define PARSE_DEBUG 1
#define PER_FRAME_EQN_DEBUG 1
#define PER_PIXEL_EQN_DEBUG 1
#else
#define CUSTOM_WAVE_DEBUG 0
#define PARAM_DEBUG 0
#define PARSE_DEBUG 0
#define PER_FRAME_EQN_DEBUG 0
#define PER_PIXEL_EQN_DEBUG 0
#endif

#if (PROJECTM_LOGLEVEL == 0)
#define PROJECTM_LOG_DEBUG //comment
#define PROJECTM_LOG_CRITICAL //comment
#elif (PROJECTM_LOGLEVEL == 1)
#define PROJECTM_LOG_DEBUG //comment
#define PROJECTM_LOG_CRITICAL LOGE
#else
#define PROJECTM_LOG_DEBUG LOGE
#define PROJECTM_LOG_CRITICAL LOGE
#endif

#endif //_PROJECTM_ANDROID_CONFIG_H
