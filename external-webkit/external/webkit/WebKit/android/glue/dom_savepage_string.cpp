/*
 * Copyright (C) 2009-2010 Motorola, Inc.
 *
 * Date            Author         Comment
 * 10/19/2009      Motorola       implemented save page function
 *
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include "dom_savepage_string.h"

/*WebCore::String StringPrintf(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    WebCore::String result = WebCore::String::format(format, ap);
    va_end(ap);
    return result;
}*/

namespace webkit_glue {

static const int DEFAULT_BUFFER_SIZE = 16 * 1024;

CStringEx::CStringEx(int capacity)
{
    if(capacity <= 0){
        capacity = DEFAULT_BUFFER_SIZE;
    }
    mBuffer = new char[capacity];
    mCapacity = capacity;
    mLength = 0;
}

const char* CStringEx::data() const
{
	return mBuffer;
}

bool CStringEx::append(const WebCore::CString& s){
    if(mLength + s.length()+1>mCapacity)
		return false;

	memcpy(&(mBuffer[mLength]), s.data(), s.length());
	mLength += s.length();
	mBuffer[mLength] = '\0';
	return true;
}

/*bool CStringEx::append(const char *s, int len){
    if(len <= 0)
        return false;

    if(mLength + len + 1 > mCapacity){
        int total = mLength + len + 1;
        total *= 2;
        char *tmp = new char[total];
        memcpy(tmp, mBuffer, mLength);
        delete []mBuffer;
        mBuffer = tmp;
        mCapacity = total;
    }

	memcpy(&(mBuffer[mLength]), s, len);
	mLength += len;
	mBuffer[mLength] = '\0';
	return true;
}*/
}
