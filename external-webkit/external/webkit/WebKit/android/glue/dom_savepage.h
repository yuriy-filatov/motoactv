/*
 * Copyright (C) 2009-2010 Motorola, Inc.
 *
 * Date            Author         Comment
 * 10/19/2009      Motorola       implemented save page function
 *
 */

#ifndef WEBKIT_GLUE_DOM_SAVEPAGE_H__
#define WEBKIT_GLUE_DOM_SAVEPAGE_H__

#include <wtf/ListHashSet.h>
#include <wtf/Vector.h>
#include <wtf/HashMap.h>
#include "KURL.h"
#include "basictypes.h"

namespace WebCore {
class Frame;
class Element;
class String;
}

namespace android {
	class WebViewCore;
}

namespace webkit_glue {

	int getResourceNum(WebCore::Frame* frame);

	int getTotalFrameNum(WebCore::Frame* frame);

	void setCancelSaving();

	int SaveFrame(WebCore::Frame *current_frame,
			const WebCore::String& savePath,
			WebCore::String& pagename,
			android::WebViewCore *webview);
}

#endif
