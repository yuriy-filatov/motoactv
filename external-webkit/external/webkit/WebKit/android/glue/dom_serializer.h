// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Copyright (C) 2009-2010 Motorola, Inc.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
//  Date            Author         Comment
//  10/19/2009      Motorola       implemented save page function
//

#ifndef WEBKIT_GLUE_DOM_SERIALIZER_H__
#define WEBKIT_GLUE_DOM_SERIALIZER_H__

#include <wtf/Vector.h>
#include <wtf/HashMap.h>
#include "KURL.h"
#include "basictypes.h"

#include "PlatformString.h"
#include "dom_savepage_string.h"

//class WebFrame;
//class WebFrameImpl;

namespace WebCore {
class Document;
class Element;
class Node;
class TextEncoding;
class Frame;
}

namespace android {
	class WebViewCore;
}

namespace webkit_glue {

class DomSerializerDelegate;

// This hash_map is used to map resource URL of original link to its local
// file path.
typedef WTF::HashMap<WebCore::String, WebCore::String> LinkLocalPathMap;


////////////////////////////////////////////////////////////////////////////////
struct SerializeDomParam {
    LinkLocalPathMap &linkMap;
    DomSerializerDelegate *delegate;
	const WebCore::String saveRootPath;
	WebCore::String resourcePath;
	android::WebViewCore *webview;
	int mErrorCode;

    // Constructor.
    SerializeDomParam(
            LinkLocalPathMap& linkMap,
            DomSerializerDelegate *delegate,
			const WebCore::String& savePath,
			WebCore::String& resourcePath,
			android::WebViewCore *webview);

    private:
    DISALLOW_EVIL_CONSTRUCTORS(SerializeDomParam);
};

// Get html data by serializing all frames of current page with lists
// which contain all resource links that have local copy.
// contain all saved auxiliary files included all sub frames and resources.
// This function will find out all frames and serialize them to HTML data.
// We have a data buffer to temporary saving generated html data. We will
// sequentially call WebViewDelegate::SendSerializedHtmlData once the data
// buffer is full. See comments of WebViewDelegate::SendSerializedHtmlData
// for getting more information.
class DomSerializer {
    public:
        // Do serialization action. Return false means no available frame has been
        // serialized, otherwise return true.
        bool SerializeDom();
        // The parameter specifies which frame need to be serialized.
        // The parameter recursive_serialization specifies whether we need to
        // serialize all sub frames of the specified frame or not.
        // The parameter delegate specifies the pointer of interface
        // DomSerializerDelegate provide sink interface which can receive the
        // individual chunks of data to be saved.
        // The parameter links contain original URLs of all saved links.
        // The parameter local_paths contain corresponding local file paths of all
        // saved links, which matched with vector:links one by one.
        // The parameter local_directory_name is relative path of directory which
        // contain all saved auxiliary files included all sub frames and resources.
        DomSerializer(WebCore::Frame* webframe, SerializeDomParam &param);

        // Generate the META for charset declaration.
        static WebCore::String GenerateMetaCharsetDeclaration(
                const WebCore::String& charset);
        // Generate the MOTW declaration.
        static WebCore::String GenerateMarkOfTheWebDeclaration(const WebCore::KURL& url);
        // Generate the default base tag declaration.
        static WebCore::String GenerateBaseTagDeclaration(
                const WebCore::String& base_target);

    private:
        // local_links_ include all pair of local resource path and corresponding
        // original link.
        LinkLocalPathMap local_links_;
        // Pointer of DomSerializerDelegate
        DomSerializerDelegate* delegate_;
        // Data buffer for saving result of serialized DOM data.
        CStringEx data_buffer_;

		struct SerializeDomParam *serializerParam;

        // Flag indicates current doc is html document or not. It's a cache value
        // of Document.isHTMLDocument().
        bool is_html_document;
        // Flag which indicate whether we have met document type declaration.
        bool has_doctype;
        // Flag which indicate whether will process meta issue.
        bool has_checked_meta;
        // This meta element need to be skipped when serializing DOM.
        const WebCore::Element* skip_meta_element;
        // Flag indicates we are in script or style tag.
        bool is_in_script_or_style_tag;
        // Flag indicates whether we have written xml document declaration.
        // It is only used in xml document
        bool has_doc_declaration;
        // Flag indicates whether we have added additional contents before end tag.
        // This flag will be re-assigned in each call of function
        // PostActionAfterSerializeOpenTag and it could be changed in function
        // PreActionBeforeSerializeEndTag if the function adds new contents into
        // serialization stream.
        bool has_added_contents_before_end;

        // Specified frame which need to be serialized;
        //WebFrameImpl* specified_webframeimpl_;
        WebCore::Frame* specified_webframeimpl_;

        // Before we begin serializing open tag of a element, we give the target
        // element a chance to do some work prior to add some additional data.
        WebCore::String PreActionBeforeSerializeOpenTag(
                const WebCore::Element* element,
                bool* need_skip);
        // After we finish serializing open tag of a element, we give the target
        // element a chance to do some post work to add some additional data.
        WebCore::String PostActionAfterSerializeOpenTag(
                const WebCore::Element* element);
        // Before we begin serializing end tag of a element, we give the target
        // element a chance to do some work prior to add some additional data.
        WebCore::String PreActionBeforeSerializeEndTag(
                const WebCore::Element* element,
                bool* need_skip);
        // After we finish serializing end tag of a element, we give the target
        // element a chance to do some post work to add some additional data.
        WebCore::String PostActionAfterSerializeEndTag(
                const WebCore::Element* element);
        // Save generated html content to data buffer.
        void SaveHtmlContentToBuffer(const WebCore::String& result);
        // Serialize open tag of an specified element.
        void OpenTagToString(const WebCore::Element* element);
        // Serialize end tag of an specified element.
        void EndTagToString(const WebCore::Element* element);
        // Build content for a specified node
        void BuildContentForNode(const WebCore::Node* node);

        DISALLOW_EVIL_CONSTRUCTORS(DomSerializer);
};

void CollectSubFrames(WebCore::Frame *specified_webframeimpl_,
         WTF::Vector<WebCore::Frame*> &frames_);

WebCore::Frame* GetWebFrameImplFromElement(WebCore::Element* element,
		bool* is_frame_element);

void CollectTargetFrames(WebCore::Frame *specified_webframeimpl_,
         WTF::Vector<WebCore::Frame*> &frames_);
}  // namespace webkit_glue

#endif  // WEBKIT_GLUE_DOM_SERIALIZER_H__
