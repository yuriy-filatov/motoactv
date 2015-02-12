// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Copyright (C) 2009-2010 Motorola, Inc.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
//  Date            Author         Comment
//  10/19/2009      Motorola       implemented save page function
//
//
// How we handle the base tag better.
// Current status:
// At now the normal way we use to handling base tag is
// a) For those links which have corresponding local saved files, such as
// savable CSS, JavaScript files, they will be written to relative URLs which
// point to local saved file. Why those links can not be resolved as absolute
// file URLs, because if they are resolved as absolute URLs, after moving the
// file location from one directory to another directory, the file URLs will
// be dead links.
// b) For those links which have not corresponding local saved files, such as
// links in A, AREA tags, they will be resolved as absolute URLs.
// c) We comment all base tags when serialzing DOM for the page.
// FireFox also uses above way to handle base tag.
//
// Problem:
// This way can not handle the following situation:
// the base tag is written by JavaScript.
// For example. The page "www.yahoo.com" use
// "document.write('<base href="http://www.yahoo.com/"...');" to setup base URL
// of page when loading page. So when saving page as completed-HTML, we assume
// that we save "www.yahoo.com" to "c:\yahoo.htm". After then we load the saved
// completed-HTML page, then the JavaScript will insert a base tag
// <base href="http://www.yahoo.com/"...> to DOM, so all URLs which point to
// local saved resource files will be resolved as
// "http://www.yahoo.com/yahoo_files/...", which will cause all saved  resource
// files can not be loaded correctly. Also the page will be rendered ugly since
// all saved sub-resource files (such as CSS, JavaScript files) and sub-frame
// files can not be fetched.
// Now FireFox, IE and WebKit based Browser all have this problem.
//
// Solution:
// My solution is that we comment old base tag and write new base tag:
// <base href="." ...> after the previous commented base tag. In WebKit, it
// always uses the latest "href" attribute of base tag to set document's base
// URL. Based on this behavior, when we encounter a base tag, we comment it and
// write a new base tag <base href="."> after the previous commented base tag.
// The new added base tag can help engine to locate correct base URL for
// correctly loading local saved resource files. Also I think we need to inherit
// the base target value from document object when appending new base tag.
// If there are multiple base tags in original document, we will comment all old
// base tags and append new base tag after each old base tag because we do not
// know those old base tags are original content or added by JavaScript. If
// they are added by JavaScript, it means when loading saved page, the script(s)
// will still insert base tag(s) to DOM, so the new added base tag(s) can
// override the incorrect base URL and make sure we alway load correct local
// saved resource files.

#define LOG_TAG "dom_ser"
#include "config.h"
#include <utils/Log.h>

#include "DocumentType.h"
#include "FrameLoader.h"
#include "Document.h"
#include "Element.h"
#include "HTMLAllCollection.h" // Motorola, amp072, 06/01/2010, IKMPSTHREEV-29 / froyo upmerge for project of external/webkit
#include "HTMLElement.h"
#include "HTMLFormElement.h"
#include "HTMLMetaElement.h"
#include "HTMLNames.h"
#include "HTMLLinkElement.h"
#include "HTMLInputElement.h"
#include "HTMLFrameOwnerElement.h"
#include "KURL.h"
#include "markup.h"
#include "TextEncoding.h"


#include "dom_serializer.h"
#include "Frame.h"
//#include "dom_savepage_string.h"
#include "dom_serializer_delegate.h"
#include "entity_map.h"
#include "dom_savepage.h"
#include "WebViewCore.h"

namespace {

// Default "mark of the web" declaration
static WebCore::String const kDefaultMarkOfTheWeb =
    "\n<!-- saved from url=(%04d)%s -->\n";

// Default meat content for writing correct charset declaration.
static WebCore::String const kDefaultMetaContent =
    "<META http-equiv=\"Content-Type\" content=\"text/html; charset=%ls\">";

// Notation of start comment.
static WebCore::String const kStartCommentNotation = "<!-- ";

// Notation of end comment.
static WebCore::String const kEndCommentNotation = " -->";

// Default XML declaration.
static WebCore::String const kXMLDeclaration =
    "<?xml version=\"%ls\" encoding=\"%ls\"%ls?>\n";

// Default base tag declaration
static WebCore::String const kBaseTagDeclaration =
    "<BASE href=\".\"%ls>";

static WebCore::String const kBaseTargetDeclaration =
    " target=\"%ls\"";

// Maximum length of data buffer which is used to temporary save generated
// html content data.
static const int kHtmlContentBufferLength = 65536/2;

// Check whether specified unicode has corresponding html/xml entity name.
// If yes, replace the character with the returned entity notation, if not
// then still use original character.
void ConvertCorrespondingSymbolToEntity(WebCore::String* result,
                                        const WebCore::String& value,
                                        bool in_html_doc) {
  unsigned len = value.length();
  const UChar* start_pos = value.characters();
  const UChar* cur_pos = start_pos;

  while (len--) {
    const char* entity_name =
        webkit_glue::EntityMap::GetEntityNameByCode(*cur_pos, in_html_doc);
    if (entity_name) {
      // Append content before entity code.
      if (cur_pos > start_pos)
        result->append(start_pos, cur_pos - start_pos);
      result->append("&");
      result->append(entity_name);
      result->append(";");
      start_pos = ++cur_pos;
    } else {
      cur_pos++;
    }
  }

  // Append the remaining content.
  if (cur_pos > start_pos)
    result->append(start_pos, cur_pos - start_pos);
}

bool ElementHasLegalLinkAttribute(const WebCore::Element* element,
		const WebCore::QualifiedName& attr_name) {
	if (attr_name == WebCore::HTMLNames::srcAttr) {
		if (element->hasTagName(WebCore::HTMLNames::imgTag) ||
				element->hasTagName(WebCore::HTMLNames::scriptTag) ||
				element->hasTagName(WebCore::HTMLNames::iframeTag) ||
				element->hasTagName(WebCore::HTMLNames::frameTag))
			return true;
		if (element->hasTagName(WebCore::HTMLNames::inputTag)) {
			const WebCore::HTMLInputElement* input =
				static_cast<const WebCore::HTMLInputElement*>(element);
			if (input->inputType() == WebCore::HTMLInputElement::IMAGE)
				return true;
		}
	} else if (attr_name == WebCore::HTMLNames::hrefAttr) {
		if (element->hasTagName(WebCore::HTMLNames::linkTag) ||
				element->hasTagName(WebCore::HTMLNames::aTag) ||
				element->hasTagName(WebCore::HTMLNames::areaTag))
			return true;
	} else if (attr_name == WebCore::HTMLNames::actionAttr) {
		if (element->hasTagName(WebCore::HTMLNames::formTag))
			return true;
	} else if (attr_name == WebCore::HTMLNames::backgroundAttr) {
		if (element->hasTagName(WebCore::HTMLNames::bodyTag) ||
				element->hasTagName(WebCore::HTMLNames::tableTag) ||
				element->hasTagName(WebCore::HTMLNames::trTag) ||
				element->hasTagName(WebCore::HTMLNames::tdTag))
			return true;
	} else if (attr_name == WebCore::HTMLNames::citeAttr) {
		if (element->hasTagName(WebCore::HTMLNames::blockquoteTag) ||
				element->hasTagName(WebCore::HTMLNames::qTag) ||
				element->hasTagName(WebCore::HTMLNames::delTag) ||
				element->hasTagName(WebCore::HTMLNames::insTag))
			return true;
	} else if (attr_name == WebCore::HTMLNames::classidAttr ||
			attr_name == WebCore::HTMLNames::dataAttr) {
		if (element->hasTagName(WebCore::HTMLNames::objectTag))
			return true;
	} else if (attr_name == WebCore::HTMLNames::codebaseAttr) {
		if (element->hasTagName(WebCore::HTMLNames::objectTag) ||
				element->hasTagName(WebCore::HTMLNames::appletTag))
			return true;
	}
	return false;
}


}  // namespace

namespace webkit_glue {

// SerializeDomParam Constructor.
SerializeDomParam::SerializeDomParam(
        LinkLocalPathMap& linkMap,
        DomSerializerDelegate *delegate,
        const WebCore::String& savePath,
		WebCore::String& resourcePath_,
		android::WebViewCore *webview)
    : linkMap(linkMap),
      delegate(delegate),
      saveRootPath(savePath),
	  resourcePath(resourcePath_),
	  webview(webview)
{
	mErrorCode = 0;
}

// Static
WebCore::String DomSerializer::GenerateMetaCharsetDeclaration(
    const WebCore::String& charset) {

    LOGSPE("GenerateMetaCharsetDeclaration1=%s", charset.utf8().data());
    WebCore::String s = "<META http-equiv=\"Content-Type\" content=\"text/html; charset="+charset+"\">";
    LOGSPE("GenerateMetaCharsetDeclaration2=%s", s.utf8().data());
    return s;
}

// Static.
WebCore::String DomSerializer::GenerateMarkOfTheWebDeclaration(
    const WebCore::KURL& url) {
    WebCore::String s = "\n<!-- saved from url=("+WebCore::String::number(url.string().length())+")"+url+" -->\n";
    return s;
  //return StringPrintf(kDefaultMarkOfTheWeb.utf8().data(),
  //                    url.string().length(), url.string().utf8().data());
}

// Static.
WebCore::String DomSerializer::GenerateBaseTagDeclaration(const WebCore::String &base_target) {
 // WebCore::String target_declaration = base_target.isEmpty() ? "" :
 //     StringPrintf(kBaseTargetDeclaration.utf8().data(), base_target.utf8().data());
  return "";//StringPrintf(kBaseTagDeclaration.utf8().data(), target_declaration.utf8().data());
}

WebCore::String DomSerializer::PreActionBeforeSerializeOpenTag(
    const WebCore::Element* element, bool* need_skip) {
  WebCore::String result;

  *need_skip = false;
  if (is_html_document) {
    // Skip the open tag of original META tag which declare charset since we
    // have overrided the META which have correct charset declaration after
    // serializing open tag of HEAD element.
    if (element->hasTagName(WebCore::HTMLNames::metaTag)) {
      const WebCore::HTMLMetaElement* meta =
          static_cast<const WebCore::HTMLMetaElement*>(element);
      // Check whether the META tag has declared charset or not.
      WebCore::String equiv = meta->httpEquiv();
      if (equalIgnoringCase(equiv, "content-type")) {
        WebCore::String content = meta->content();
        if (content.length() && content.contains("charset", false)) {
          // Find META tag declared charset, we need to skip it when
          // serializing DOM.
          skip_meta_element = element;
          *need_skip = true;
        }
      }
    } else if (element->hasTagName(WebCore::HTMLNames::htmlTag)) {
      // Check something before processing the open tag of HEAD element.
      // First we add doc type declaration if original doc has it.
      if (!has_doctype) {
        has_doctype = true;
        result += createMarkup(specified_webframeimpl_->document()->doctype());
      }

      // Add MOTW declaration before html tag.
      // See http://msdn2.microsoft.com/en-us/library/ms537628(VS.85).aspx.
      result += GenerateMarkOfTheWebDeclaration(specified_webframeimpl_->loader()->url());
    } else if (element->hasTagName(WebCore::HTMLNames::baseTag)) {
      // Comment the BASE tag when serializing dom.
      result += kStartCommentNotation;
    }
  } else {
    // Write XML declaration.
    if (!has_doc_declaration) {
      has_doc_declaration = true;
      // Get encoding info.
      WebCore::String xml_encoding = specified_webframeimpl_->document()->xmlEncoding();
      if (xml_encoding.isEmpty())
        xml_encoding = specified_webframeimpl_->loader()->encoding();
      if (xml_encoding.isEmpty())
        xml_encoding = WebCore::UTF8Encoding().name();

        WebCore::String standalone = specified_webframeimpl_->document()->xmlStandalone()?" standalone=\"yes\"" :"";
        WebCore::String str_xml_declaration =
            "<?xml version=\""+specified_webframeimpl_->document()->xmlVersion()+
            "\" encoding=\""+xml_encoding+
            "\""+standalone + "?>\n";

      result += str_xml_declaration;
    }
    // Add doc type declaration if original doc has it.
    if (!has_doctype) {
      has_doctype = true;
      result += createMarkup(specified_webframeimpl_->document()->doctype());
    }
  }

  return result;
}

WebCore::String DomSerializer::PostActionAfterSerializeOpenTag(
    const WebCore::Element* element) {
  WebCore::String result;

  has_added_contents_before_end = false;
  if (!is_html_document)
    return result;
  // Check after processing the open tag of HEAD element
  if (!has_checked_meta &&
      element->hasTagName(WebCore::HTMLNames::headTag)) {
    has_checked_meta = true;
    // Check meta element. WebKit only pre-parse the first 512 bytes
    // of the document. If the whole <HEAD> is larger and meta is the
    // end of head part, then this kind of pages aren't decoded correctly
    // because of this issue. So when we serialize the DOM, we need to
    // make sure the meta will in first child of head tag.
    // See http://bugs.webkit.org/show_bug.cgi?id=16621.
    // First we generate new content for writing correct META element.
    WebCore::String str_meta =
        GenerateMetaCharsetDeclaration(specified_webframeimpl_->loader()->encoding());
    result += str_meta;

    has_added_contents_before_end = true;
    // Will search each META which has charset declaration, and skip them all
    // in PreActionBeforeSerializeOpenTag.
  } else if (element->hasTagName(WebCore::HTMLNames::scriptTag) ||
             element->hasTagName(WebCore::HTMLNames::styleTag)) {
    is_in_script_or_style_tag = true;
  }

  return result;
}

WebCore::String DomSerializer::PreActionBeforeSerializeEndTag(
    const WebCore::Element* element,
    bool* need_skip) {
  WebCore::String result;

  *need_skip = false;
  if (!is_html_document)
    return result;
  // Skip the end tag of original META tag which declare charset.
  // Need not to check whether it's META tag since we guarantee
  // skip_meta_element is definitely META tag if it's not NULL.
  if (skip_meta_element == element) {
    *need_skip = true;
  } else if (element->hasTagName(WebCore::HTMLNames::scriptTag) ||
             element->hasTagName(WebCore::HTMLNames::styleTag)) {
    //DCHECK(is_in_script_or_style_tag);
    is_in_script_or_style_tag = false;
  }

  return result;
}

// After we finish serializing end tag of a element, we give the target
// element a chance to do some post work to add some additional data.
WebCore::String DomSerializer::PostActionAfterSerializeEndTag(
    const WebCore::Element* element) {
  WebCore::String result;

  if (!is_html_document)
    return result;
  // Comment the BASE tag when serializing DOM.
  if (element->hasTagName(WebCore::HTMLNames::baseTag)) {
    result += kEndCommentNotation;
    // Append a new base tag declaration.
    result += GenerateBaseTagDeclaration(specified_webframeimpl_->document()->baseTarget());
  }

  return result;
}

    void DomSerializer::SaveHtmlContentToBuffer(const WebCore::String& result) {
        if (!result.length())
            return;
        // Convert the unicode content to target encoding

        WebCore::String encoding(specified_webframeimpl_->loader()->encoding());

        WebCore::TextEncoding text_encoding(encoding);
        text_encoding = encoding.length() ? text_encoding : WebCore::UTF8Encoding();

        WebCore::CString encoding_result = text_encoding.encode(
                result.characters(), result.length(), WebCore::EntitiesForUnencodables);

        // if the data buffer will be full, then send it out first.
        if (encoding_result.length() + data_buffer_.length() > (unsigned int)(data_buffer_.capacity())) {
            // Send data to delegate, tell it now we are serializing current frame.
            int result = delegate_->DidSerializeDataForFrame(specified_webframeimpl_,
                    data_buffer_.data(), DomSerializerDelegate::CURRENT_FRAME_IS_NOT_FINISHED);
            data_buffer_.clear();
            if(result != 0){
                serializerParam->mErrorCode = result;
                return;
            }
        }

        // Append result to data buffer.
        if(data_buffer_.append(encoding_result) == false){
            if(data_buffer_.length()>0){
                int result = delegate_->DidSerializeDataForFrame(specified_webframeimpl_,
                        data_buffer_.data(), DomSerializerDelegate::CURRENT_FRAME_IS_NOT_FINISHED);

                if(result != 0){
                    serializerParam->mErrorCode = result;
                    return;
                }
                data_buffer_.clear();
            }

            int result = delegate_->DidSerializeDataForFrame(specified_webframeimpl_,
                                            encoding_result.data(), DomSerializerDelegate::CURRENT_FRAME_IS_NOT_FINISHED);

            if(result != 0){
                serializerParam->mErrorCode = result;
                return;
            }
        }
    }

void DomSerializer::OpenTagToString(const WebCore::Element* element) {
  bool need_skip;
  // Do pre action for open tag.
  WebCore::String result = PreActionBeforeSerializeOpenTag(element, &need_skip);
  if (need_skip)
    return;
  // Add open tag
  result += "<" + element->nodeName();
  // Go through all attributes and serialize them.
  const WebCore::NamedNodeMap *attrMap = element->attributes(true);
  if (attrMap) {
    unsigned numAttrs = attrMap->length();
    for (unsigned i = 0; i < numAttrs; i++) {
      result += " ";
      // Add attribute pair
      const WebCore::Attribute *attribute = attrMap->attributeItem(i);
      result += attribute->name().toString();
      result += "=\"";
      if (!attribute->value().isEmpty()) {
        // Check whether we need to replace some resource links
        // with local resource paths.
        const WebCore::QualifiedName& attr_name = attribute->name();
        // Check whether need to change the attribute which has link
        bool need_replace_link =
            ElementHasLegalLinkAttribute(element, attr_name);
        if (need_replace_link) {

          // First, get the absolute link
          const WebCore::String& attr_value = attribute->value();
          // For links start with "javascript:", we do not change it.
          if (attr_value.startsWith("javascript:", false)) {
            result += attr_value;
          } else {
              bool isFrameElement;
              WebCore::String str_value;
              WebCore::Element *e = (WebCore::Element*)element;

              const WebCore::Frame* frame = GetWebFrameImplFromElement(e, &isFrameElement);
              if(frame != NULL && frame->loader()->url().string().length() != 0){
                  str_value = frame->loader()->url().string();
                  LOGSPE("attr_value111=%s\n", str_value.utf8().data());
              }
              else{
                  str_value = specified_webframeimpl_->document()->completeURL(attr_value);
              }

              //WebCore::String str_value = specified_webframeimpl_->document()->completeURL(attr_value);
              WebCore::String value(str_value);
              //LOGSPE("attr_value=%s\n", str_value.utf8().data());
              //Check whether we local files for those link.
              LinkLocalPathMap::iterator it = local_links_.find(value);
              if (it != local_links_.end()) {
                  // Replace the link when we have local files.
                  result += it->second;
              } else {
                  // If not found local path, replace it with absolute link.
                  result += str_value;
              }
          }
        } else {
          ConvertCorrespondingSymbolToEntity(&result, attribute->value(),
              is_html_document);
        }
      }
      result += "\"";
    }
    //LOGSPE("end of OpenTagToString loop");
  }

  // Do post action for open tag.
  WebCore::String added_contents =
      PostActionAfterSerializeOpenTag(element);
  // Complete the open tag for element when it has child/children.
  if (element->hasChildNodes() || has_added_contents_before_end)
    result += ">";
  // Append the added contents generate in  post action of open tag.
  result += added_contents;
  //LOGSPE("Enter SaveHtmlContentToBuffer");
  // Save the result to data buffer.
  SaveHtmlContentToBuffer(result);

  //LOGSPE("end of OpenTagToString");
}

// Serialize end tag of an specified element.
void DomSerializer::EndTagToString(const WebCore::Element* element) {
  bool need_skip;
  // Do pre action for end tag.
  WebCore::String result = PreActionBeforeSerializeEndTag(element,
                                                          &need_skip);
  if (need_skip)
    return;
  // Write end tag when element has child/children.
  if (element->hasChildNodes() || has_added_contents_before_end) {
    result += "</";
    result += element->nodeName();
    result += ">";
  } else {
    // Check whether we have to write end tag for empty element.
    if (is_html_document) {
      result += ">";
      const WebCore::HTMLElement* html_element =
          static_cast<const WebCore::HTMLElement*>(element);
      if (html_element->endTagRequirement() == WebCore::TagStatusRequired) {
        // We need to write end tag when it is required.
        result += "</";
        result += html_element->nodeName();
        result += ">";
      }
    } else {
      // For xml base document.
      result += " />";
    }
  }
  // Do post action for end tag.
  result += PostActionAfterSerializeEndTag(element);
  // Save the result to data buffer.
  SaveHtmlContentToBuffer(result);
}

void DomSerializer::BuildContentForNode(const WebCore::Node* node) {
  switch (node->nodeType()) {
    case WebCore::Node::ELEMENT_NODE: {
      // Process open tag of element.
      OpenTagToString(static_cast<const WebCore::Element*>(node));
      // Walk through the children nodes and process it.
      for (const WebCore::Node *child = node->firstChild(); child != NULL && serializerParam->mErrorCode == 0 ;
           child = child->nextSibling())
        BuildContentForNode(child);

      // Process end tag of element.
      EndTagToString(static_cast<const WebCore::Element*>(node));
      break;
    }
    case WebCore::Node::TEXT_NODE: {
      SaveHtmlContentToBuffer(createMarkup(node));
      break;
    }
    case WebCore::Node::ATTRIBUTE_NODE:
    case WebCore::Node::DOCUMENT_NODE:
    case WebCore::Node::DOCUMENT_FRAGMENT_NODE: {
      // Should not exist.
      //DCHECK(false);
      break;
    }
    // Document type node can be in DOM?
    case WebCore::Node::DOCUMENT_TYPE_NODE:
      has_doctype = true;
    default: {
      // For other type node, call default action.
      SaveHtmlContentToBuffer(createMarkup(node));
      break;
    }
  }
}

DomSerializer::DomSerializer(WebCore::Frame* webframe,
                             SerializeDomParam &param)
    : local_links_(param.linkMap),
      delegate_(param.delegate),
	  data_buffer_(kHtmlContentBufferLength),
	  serializerParam(&param),
      has_doctype(false),
      has_checked_meta(false),
      skip_meta_element(NULL),
      is_in_script_or_style_tag(false),
      has_doc_declaration(false),
      has_added_contents_before_end(false) // Motorola, W21932, 2010/09/10, IKMAP-7163 / KW issue: local Variable is used uninitialized.
{
  specified_webframeimpl_ = webframe;
  is_html_document = webframe->document()->isHTMLDocument();

  int i=0;
  for (LinkLocalPathMap::iterator it = local_links_.begin();
          it != local_links_.end(); ++it) {
      LOGSPE("[%d]%s==>%s\n", i++, it->first.utf8().data(), it->second.utf8().data());
  }
}

bool DomSerializer::SerializeDom() {

    LOGSPE("Enter SerializeDom delegate=%p, current_frame=%p\n", delegate_, specified_webframeimpl_);

    WebCore::Document* current_doc = specified_webframeimpl_->document();

    data_buffer_.clear();
    // Process current document.
    WebCore::Element* root_element = current_doc->documentElement();
    if (root_element)
        BuildContentForNode(root_element);

    // Sink the remainder data and finish serializing current frame.
    int result = delegate_->DidSerializeDataForFrame(specified_webframeimpl_, data_buffer_.data(),
            DomSerializerDelegate::CURRENT_FRAME_IS_FINISHED);

    if(result != 0){
        serializerParam->mErrorCode = result;
    }

	serializerParam->webview->NotifyOneResourceDone();

    // Clear the buffer.
    data_buffer_.clear();
    LOGSPE("Exit result=%d, SerializeDom delegate=%p, current_frame=%p\n", result, delegate_, specified_webframeimpl_);

    return true;
}

WebCore::Frame* GetWebFrameImplFromElement(WebCore::Element* element,
		bool* is_frame_element) {
	*is_frame_element = false;
	if (element->hasTagName(WebCore::HTMLNames::iframeTag) ||
			element->hasTagName(WebCore::HTMLNames::frameTag)) {
		*is_frame_element = true;
		if (element->isFrameOwnerElement()) {
			WebCore::HTMLFrameOwnerElement* frame_element =
				static_cast<WebCore::HTMLFrameOwnerElement*>(element);
			WebCore::Frame* content_frame = frame_element->contentFrame();
			return content_frame;// ? WebFrameImpl::FromFrame(content_frame) : NULL;
		}
	}
	return NULL;
}

void CollectSubFrames(WebCore::Frame *specified_webframeimpl_,
         WTF::Vector<WebCore::Frame*> &frames_)
{
  // Collect all frames inside the specified frame.

    WebCore::Frame* current_frame = specified_webframeimpl_;
    // Get current using document.
    WebCore::Document* current_doc = current_frame->document();
    // Go through sub-frames.
    RefPtr<WebCore::HTMLAllCollection> all = current_doc->all(); // Motorola, amp072, 06/01/2010, IKMPSTHREEV-29 / froyo upmerge for project of external/webkit
    for (WebCore::Node* node = all->firstItem(); node != NULL;
         node = all->nextItem()) {
      if (!node->isHTMLElement())
        continue;
      WebCore::Element* element = static_cast<WebCore::Element*>(node);
      // Check frame tag and iframe tag.
      bool is_frame_element;
      WebCore::Frame* web_frame = GetWebFrameImplFromElement(element, &is_frame_element);
      if (is_frame_element && web_frame){
        frames_.append(web_frame);
		//CollectTargetFrames(web_frame, frames_, recursive_serialization_);
	  }
    }
}

void CollectTargetFrames(WebCore::Frame *specified_webframeimpl_,
         WTF::Vector<WebCore::Frame*> &frames_) {

  // First, process main frame.
  frames_.append(specified_webframeimpl_);

  // Collect all frames inside the specified frame.
  for (int i = 0; i < static_cast<int>(frames_.size()); ++i) {
    WebCore::Frame* current_frame = frames_[i];
    // Get current using document.
    WebCore::Document* current_doc = current_frame->document();
    // Go through sub-frames.
    RefPtr<WebCore::HTMLAllCollection> all = current_doc->all(); // Motorola, amp072, 06/01/2010, IKMPSTHREEV-29 / froyo upmerge for project of external/webkit
    for (WebCore::Node* node = all->firstItem(); node != NULL;
         node = all->nextItem()) {
      if (!node->isHTMLElement())
        continue;
      WebCore::Element* element = static_cast<WebCore::Element*>(node);
      // Check frame tag and iframe tag.
      bool is_frame_element;
      WebCore::Frame* web_frame = GetWebFrameImplFromElement(element, &is_frame_element);
      if (is_frame_element && web_frame){
        frames_.append(web_frame);
	  }
    }
  }
}

}  // namespace webkit_glue
