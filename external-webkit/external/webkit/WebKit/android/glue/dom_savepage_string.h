/*
 * Copyright (C) 2009-2010 Motorola, Inc.
 *
 * Date            Author         Comment
 * 10/19/2009      Motorola       implemented save page function
 *
 */


#ifndef DOM_SAVEPAGE_STRING_UTIL_H_
#define DOM_SAVEPAGE_STRING_UTIL_H_

#include "PlatformString.h"
#include <wtf/Vector.h>
#include "CString.h"

//WebCore::String StringPrintf(const char* format, ...);

// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
// so we don't want to use it here.
template <class Char> inline Char ToLowerASCII(Char c) {
	return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

template <class str> inline void StringToLowerASCII(str* s) {
	for (typename str::iterator i = s->begin(); i != s->end(); ++i)
		*i = ToLowerASCII(*i);
}

namespace webkit_glue {
	class CStringEx {
		public:
			CStringEx(int capacity);

			~CStringEx(){
				if(mBuffer != 0){
					delete [] mBuffer;
				}
			}

			bool append(const WebCore::CString &s);
			//bool append(const char *s, int len);
			int length() {return mLength;}
			int capacity() {return mCapacity;}
			void clear() {mLength = 0;}

			const char* data() const;
			unsigned length() const;

		protected:
			int mLength;
			int mCapacity;
			char *mBuffer;

	};

}

#define DBG_SAVEPAGE_PRINTF 0
#if DBG_SAVEPAGE_PRINTF
#include <stdio.h>
extern FILE* gSavePageLogFile;
#define SAVEPAGE_LOG_FILE "/data/data/com.android.browser/savelog"

#define OPENSPLOG() do {                                      \
            if (!gSavePageLogFile)                                \
                gSavePageLogFile = fopen(SAVEPAGE_LOG_FILE, "a"); \
        } while (false)

#define CLOSESPLOG() do {                               \
            if (gSavePageLogFile) {                         \
                            fclose(gSavePageLogFile);                   \
                            gSavePageLogFile = NULL;                    \
                        }                                               \
        } while (false)

#define LOGSP(...) do {                               \
            OPENSPLOG();                                  \
            if (gSavePageLogFile)                         \
                fprintf(gSavePageLogFile, __VA_ARGS__);   \
        } while (false)

#define LOGSPE(...) do {                                   \
            OPENSPLOG();                                       \
            if (gSavePageLogFile)                              \
                fprintf(gSavePageLogFile, __VA_ARGS__);        \
            CLOSESPLOG();                                      \
        } while (false)

#else
#define OPENSPLOG()
#define CLOSESPLOG()
#define LOGSP(...) ((void)0)
#define LOGSPE(...) ((void)0)
#endif

#endif
