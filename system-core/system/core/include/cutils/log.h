/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// C/C++ logging functions.  See the logging documentation for API details.
//
// We'd like these to be available from C code (in case we import some from
// somewhere), so this has a C interface.
//
// The output will be correct when the log file is shared between multiple
// threads and/or multiple processes so long as the operating system
// supports O_APPEND.  These calls have mutex-protected data structures
// and so are NOT reentrant.  Do not use LOG in a signal handler.
//
#ifndef _LIBS_CUTILS_LOG_H
#define _LIBS_CUTILS_LOG_H

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <stdarg.h>

#include <cutils/uio.h>
#include <cutils/logd.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------

/*
 * Normally we strip LOGV (VERBOSE messages) from release builds.
 * You can modify this (for example with "#define LOG_NDEBUG 0"
 * at the top of your source file) to change that behavior.
 */
#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

// ---------------------------------------------------------------------

/*
 * Simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef LOGV
#if LOG_NDEBUG
#define LOGV(...)   ((void)0)
#else
#define LOGV(...) ((void)LOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

#ifndef LOGV_IF
#if LOG_NDEBUG
#define LOGV_IF(cond, ...)   ((void)0)
#else
#define LOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

/*
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef LOGD
#define LOGD(...) ((void)LOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGD_IF
#define LOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef LOGI
#define LOGI(...) ((void)LOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGI_IF
#define LOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef LOGW
#define LOGW(...) ((void)LOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGW_IF
#define LOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef LOGE
#define LOGE(...) ((void)LOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGE_IF
#define LOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

// ---------------------------------------------------------------------

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * verbose priority.
 */
#ifndef IF_LOGV
#if LOG_NDEBUG
#define IF_LOGV() if (false)
#else
#define IF_LOGV() IF_LOG(LOG_VERBOSE, LOG_TAG)
#endif
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * debug priority.
 */
#ifndef IF_LOGD
#define IF_LOGD() IF_LOG(LOG_DEBUG, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * info priority.
 */
#ifndef IF_LOGI
#define IF_LOGI() IF_LOG(LOG_INFO, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * warn priority.
 */
#ifndef IF_LOGW
#define IF_LOGW() IF_LOG(LOG_WARN, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * error priority.
 */
#ifndef IF_LOGE
#define IF_LOGE() IF_LOG(LOG_ERROR, LOG_TAG)
#endif


// ---------------------------------------------------------------------

/*
 * Simplified macro to send a verbose system log message using the current LOG_TAG.
 */
#ifndef SLOGV
#if LOG_NDEBUG
#define SLOGV(...)   ((void)0)
#else
#define SLOGV(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

#ifndef SLOGV_IF
#if LOG_NDEBUG
#define SLOGV_IF(cond, ...)   ((void)0)
#else
#define SLOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

/*
 * Simplified macro to send a debug system log message using the current LOG_TAG.
 */
#ifndef SLOGD
#define SLOGD(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGD_IF
#define SLOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an info system log message using the current LOG_TAG.
 */
#ifndef SLOGI
#define SLOGI(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGI_IF
#define SLOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send a warning system log message using the current LOG_TAG.
 */
#ifndef SLOGW
#define SLOGW(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGW_IF
#define SLOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an error system log message using the current LOG_TAG.
 */
#ifndef SLOGE
#define SLOGE(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGE_IF
#define SLOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

    

// ---------------------------------------------------------------------

/*
 * Log a fatal error.  If the given condition fails, this stops program
 * execution like a normal assertion, but also generating the given message.
 * It is NOT stripped from release builds.  Note that the condition test
 * is -inverted- from the normal assert() semantics.
 */
#define LOG_ALWAYS_FATAL_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)android_printAssert(#cond, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )

#define LOG_ALWAYS_FATAL(...) \
    ( ((void)android_printAssert(NULL, LOG_TAG, __VA_ARGS__)) )

/*
 * Versions of LOG_ALWAYS_FATAL_IF and LOG_ALWAYS_FATAL that
 * are stripped out of release builds.
 */
#if LOG_NDEBUG

#define LOG_FATAL_IF(cond, ...) ((void)0)
#define LOG_FATAL(...) ((void)0)

#else

#define LOG_FATAL_IF(cond, ...) LOG_ALWAYS_FATAL_IF(cond, __VA_ARGS__)
#define LOG_FATAL(...) LOG_ALWAYS_FATAL(__VA_ARGS__)

#endif

/*
 * Assertion that generates a log message when the assertion fails.
 * Stripped out of release builds.  Uses the current LOG_TAG.
 */
#define LOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), __VA_ARGS__)
//#define LOG_ASSERT(cond) LOG_FATAL_IF(!(cond), "Assertion failed: " #cond)

// ---------------------------------------------------------------------

/*
 * Basic log message macro.
 *
 * Example:
 *  LOG(LOG_WARN, NULL, "Failed with error %d", errno);
 *
 * The second argument may be NULL or "" to indicate the "global" tag.
 */
#ifndef LOG
#define LOG(priority, tag, ...) \
    LOG_PRI(ANDROID_##priority, tag, __VA_ARGS__)
#endif

/*
 * Log macro that allows you to specify a number for the priority.
 */
#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) \
    android_printLog(priority, tag, __VA_ARGS__)
#endif

/*
 * Log macro that allows you to pass in a varargs ("args" is a va_list).
 */
#ifndef LOG_PRI_VA
#define LOG_PRI_VA(priority, tag, fmt, args) \
    android_vprintLog(priority, NULL, tag, fmt, args)
#endif

/*
 * Conditional given a desired logging priority and tag.
 */
#ifndef IF_LOG
#define IF_LOG(priority, tag) \
    if (android_testLog(ANDROID_##priority, tag))
#endif

// ---------------------------------------------------------------------

/*
 * KPI Logging
 */

typedef struct {
    char *key;
    char *value;
}KEY_VALUE_T;

typedef struct {
    int numKeyValuePairs;
    KEY_VALUE_T **keyvalues;
}EXTRALOGS_T;

#define LOG_TAG_L1 "MOT_DEVICE_KPI_L1"
#define LOG_TAG_L2 "MOT_DEVICE_KPI_L2"
#define LOG_TAG_L3 "MOT_DEVICE_KPI_L3"

#define KPI_VERSION "1.0"

#define KPI_P 1
#define KPI_E 2

#define LEVEL_1 1
#define LEVEL_2 2
#define LEVEL_3 3

#define ALLOC_EXTRALOGS(_e, _size)                                        \
    _e = (EXTRALOGS_T *)malloc(sizeof(EXTRALOGS_T));                      \
    _e->numKeyValuePairs = _size;                                         \
    _e->keyvalues = (KEY_VALUE_T **)malloc(sizeof(KEY_VALUE_T *) * _size);

#define SET_KEYVALUE(_kv, _k, _v)                                         \
    _kv = (KEY_VALUE_T*)malloc(sizeof(KEY_VALUE_T));                      \
    _kv->key = (char *)malloc(sizeof(char) * (strlen(_k) + 1));           \
    strcpy(_kv->key, _k);                                                 \
    _kv->value = (char *)malloc(sizeof(char) * (strlen(_v) + 1));         \
    strcpy(_kv->value, _v);

#define FREE_EXTRALOGS(_e)                                                \
    int _i;                                                               \
    for (_i=0; _i<_e->numKeyValuePairs; _i++) {                           \
        free(_e->keyvalues[_i]->key);                                     \
        free(_e->keyvalues[_i]->value);                                   \
        free(_e->keyvalues[_i]);                                          \
    }                                                                     \
    free(_e->keyvalues);                                                  \
    free(_e);                                                             \
    _e = NULL;

#define LOG_KPI(_logtype, _level, _component, _action, _extralogs) {      \
    char _propBuf[PROPERTY_VALUE_MAX];                                    \
    property_get("ro.build.type", _propBuf, "user");                      \
    if (((strcmp(_propBuf, "user") == 0 ) && (_logtype == KPI_P))         \
        || (strcmp(_propBuf, "user") != 0)) {                             \
    struct timeval _tv;                                                   \
    int _i = 0;                                                           \
    int _len = 0;                                                         \
    EXTRALOGS_T *_extras = _extralogs;                                    \
    if (_extras != NULL) {                                                \
        for (_i=0; _i< _extras->numKeyValuePairs; _i++) {                 \
            _len += strlen(_extras->keyvalues[_i]->key);                  \
            _len += strlen(_extras->keyvalues[_i]->value);                \
            _len += 2;                                                    \
        }                                                                 \
    }                                                                     \
    gettimeofday(&_tv, (struct timezone *) NULL);                         \
    long long _time = _tv.tv_sec * 1000LL + _tv.tv_usec / 1000;           \
    int _hdrlen = 85;                                                     \
    char *_logbuf = (char *) malloc(sizeof(char) *                        \
          (_hdrlen + strlen(_component) + strlen(_action) + _len + 1));   \
    *_logbuf='\0';                                                        \
    sprintf(_logbuf, "[ID=KPI;v=%s;t=%lld;c=%s;a=%s;]",                   \
          KPI_VERSION, _time, _component, _action);                       \
    if (_len > 0) {                                                       \
        strcat(_logbuf, "[ID=ext;");                                      \
        for (_i=0; _i< _extras->numKeyValuePairs; _i++) {                 \
            strcat(_logbuf, _extras->keyvalues[_i]->key);                 \
            strcat(_logbuf, "=");                                         \
            strcat(_logbuf, _extras->keyvalues[_i]->value);               \
            strcat(_logbuf, ";");                                         \
        }                                                                 \
        strcat(_logbuf, "]");                                             \
    }                                                                     \
    if (_level == LEVEL_1) {                                              \
        LOG(LOG_INFO, LOG_TAG_L1, "%s", (const char *)_logbuf);           \
    } else if (_level == LEVEL_2) {                                       \
        LOG(LOG_INFO, LOG_TAG_L2, "%s", (const char *)_logbuf);           \
    } else if (_level == LEVEL_3) {                                       \
        LOG(LOG_INFO, LOG_TAG_L3, "%s", (const char *)_logbuf);           \
    }                                                                     \
    free(_logbuf);                                                        \
    }                                                                     \
}

/*
 * Accumulated stats logging for native clients
 */
#define LOG_ACCUMSTATS_TAG "MOT_DEVICE_ACCUM_STATS"
#define LOG_ACCUMSTATS(_component, _statsName, _key, _value) {         \
    LOG(LOG_INFO, LOG_ACCUMSTATS_TAG, "%s,%s,%s,%s",                   \
        _component, _statsName, _key, _value);                         \
}

// End: KPI Logging

// ---------------------------------------------------------------------

/*
 * Event logging.
 */

/*
 * Event log entry types.  These must match up with the declarations in
 * java/android/android/util/EventLog.java.
 */
typedef enum {
    EVENT_TYPE_INT      = 0,
    EVENT_TYPE_LONG     = 1,
    EVENT_TYPE_STRING   = 2,
    EVENT_TYPE_LIST     = 3,
} AndroidEventLogType;


#define LOG_EVENT_INT(_tag, _value) {                                       \
        int intBuf = _value;                                                \
        (void) android_btWriteLog(_tag, EVENT_TYPE_INT, &intBuf,            \
            sizeof(intBuf));                                                \
    }
#define LOG_EVENT_LONG(_tag, _value) {                                      \
        long long longBuf = _value;                                         \
        (void) android_btWriteLog(_tag, EVENT_TYPE_LONG, &longBuf,          \
            sizeof(longBuf));                                               \
    }
#define LOG_EVENT_STRING(_tag, _value)                                      \
    ((void) 0)  /* not implemented -- must combine len with string */
/* TODO: something for LIST */

/*
 * ===========================================================================
 *
 * The stuff in the rest of this file should not be used directly.
 */

#define android_printLog(prio, tag, fmt...) \
    __android_log_print(prio, tag, fmt)

#define android_vprintLog(prio, cond, tag, fmt...) \
    __android_log_vprint(prio, tag, fmt)

#define android_printAssert(cond, tag, fmt...) \
    __android_log_assert(cond, tag, fmt)

#define android_writeLog(prio, tag, text) \
    __android_log_write(prio, tag, text)

#define android_bWriteLog(tag, payload, len) \
    __android_log_bwrite(tag, payload, len)
#define android_btWriteLog(tag, type, payload, len) \
    __android_log_btwrite(tag, type, payload, len)
	
// TODO: remove these prototypes and their users
#define android_testLog(prio, tag) (1)
#define android_writevLog(vec,num) do{}while(0)
#define android_write1Log(str,len) do{}while (0)
#define android_setMinPriority(tag, prio) do{}while(0)
//#define android_logToCallback(func) do{}while(0)
#define android_logToFile(tag, file) (0)
#define android_logToFd(tag, fd) (0)

typedef enum {
    LOG_ID_MAIN = 0,
    LOG_ID_RADIO = 1,
    LOG_ID_EVENTS = 2,
    LOG_ID_SYSTEM = 3,

    LOG_ID_MAX
} log_id_t;

/*
 * Send a simple string to the log.
 */
int __android_log_buf_write(int bufID, int prio, const char *tag, const char *text);
int __android_log_buf_print(int bufID, int prio, const char *tag, const char *fmt, ...);


#ifdef __cplusplus
}
#endif

#endif // _LIBS_CUTILS_LOG_H
