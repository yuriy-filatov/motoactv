/*
 * Copyright (C) 2009-2010 Motorola, Inc.
 *
 * Date            Author         Comment
 * 10/19/2009      Motorola       implemented save page function
 *
 */

#include "config.h"
#include <utils/Log.h>

#include "dom_savepage.h"
#include "Frame.h"
#include "DocumentType.h"
#include "FrameLoader.h"
#include "Document.h"
#include "DocLoader.h"
#include "Element.h"
#include "HTMLCollection.h"
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
#include "PlatformString.h"
#include "dom_serializer.h"
#include "dom_serializer_delegate.h"
#include "WebViewCore.h"

#include <wtf/HashMap.h>

namespace webkit_glue {

static void filterSpecialChars(WebCore::String& name)
{
    name.replace('\\', '_').replace('\"', '_').replace('\'', '_')
        .replace('&', '_').replace('!', '_').replace(' ', '_')
        .replace(':', '_').replace('=', '_').replace(',', '_')
        .replace('-', '_').replace('?', '_').replace("__", "_")
        .replace('|', '_').replace('*', '_'); // Motorola, W21932, 09/20/2010, IKMAIN-4894 / need to filter illegal char(*) in filename string
}

static void modifyFileNameIfExists(const WebCore::String& path, WebCore::String& baseName, const WebCore::String& extension)
{
    int extraInt = 0;
    WebCore::String tempFileName = path + "/" + baseName + "." + extension;

    int ret = access(tempFileName.utf8().data(), 0);
    while (!ret) { //file already exist
        extraInt ++;
        tempFileName = path + "/" + baseName + WebCore::String::number(extraInt) + "." + extension;
        ret = access(tempFileName.utf8().data(), 0);
    }

    if (extraInt > 0)
        baseName = baseName + WebCore::String::number(extraInt);
}

static int createResourceDir(const WebCore::String &resourcePath){
    int ret = access(resourcePath.utf8().data(), 0);
    if ( ret != 0) {
        int ret = mkdir(resourcePath.utf8().data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (ret) {
            LOGSPE("make resource folder failed: %s\n", resourcePath.utf8().data());
            return -1;
        }
    }
    else { //resource exists
        struct stat status;
        ret = stat(resourcePath.utf8().data(), &status);
        LOGSPE("resource folder exists. let's check... stat result=%d\n", ret);
        if (!(status.st_mode & S_IFDIR)) {
            LOGSPE("path %s is not a directory\n", resourcePath.utf8().data());
            return -1;
        }
        ret = access(resourcePath.utf8().data(), W_OK);
        if (ret != 0) {
            LOGSPE("path %s is not writable, ret=%d\n", resourcePath.utf8().data(), ret);
            return -1;
        }
    }

    return 0;
}

static int parseHex(UChar b) {
    if (b >= '0' && b <= '9') return (b - '0');
    if (b >= 'A' && b <= 'F') return (b - 'A' + 10);
    if (b >= 'a' && b <= 'f') return (b - 'a' + 10);

    return 0;
}

static WebCore::String convertFileName(WebCore::String &fileName){
    WebCore::String newName;
    bool isTransferred = false;
    for(unsigned int i= 0; i<fileName.length(); i++){
        if(fileName[i] == '%' && i+2<fileName.length()){
            isTransferred = true;
            int uchar = parseHex(fileName[i+1])*16 + parseHex(fileName[i+2]);
            // remove "/" with UChar 47 in the filenames,since filterSpecialChars() can not filter this Char.
            if(uchar == (int)'/') {
               newName.append('_'); // prevent filename from being null;
            } else {
                newName.append(UChar(uchar));
            }
            i+=2;
        }
        else{
            newName.append(fileName[i]);
        }
    }
    if (isTransferred) {
        filterSpecialChars(newName);
        return newName;
    }
    return newName;
}

static WebCore::String getAvailableBaseName(const WebCore::Frame* frame, const WebCore::String& path, bool isTop)
{
    WebCore::String fileBaseName = frame->document()->baseURI();
    if (isTop) {
        fileBaseName = frame->document()->title().stripWhiteSpace();
        if (fileBaseName.isEmpty())
            fileBaseName = frame->document()->baseURI();
    }
    int i = fileBaseName.reverseFind('/');
    if (i >= 0)
        fileBaseName.remove(0, i+1);
    i = fileBaseName.reverseFind('.');
    if (i >= 0)
        fileBaseName.remove(i, fileBaseName.length() - i);

    if (fileBaseName.isEmpty())
        fileBaseName = frame->document()->domain();

    if (fileBaseName.isEmpty())
        fileBaseName = "index";

    filterSpecialChars(fileBaseName);
    if (fileBaseName.length() > 32)
        fileBaseName.truncate(32); //keep name short

	WebCore::String extension = "html";
    modifyFileNameIfExists(path, fileBaseName, extension);

    return fileBaseName;
}

class FrameResource{

	protected:
		const WebCore::Frame* frame;
        LinkLocalPathMap &linkMap;
        struct SerializeDomParam* serializerParam;
		WebCore::String &mResourcePath;

	public:
		FrameResource(WebCore::Frame* frame_, struct SerializeDomParam* param_);
		bool save();

	protected:
		bool doSaveResources(PassRefPtr<WebCore::HTMLCollection> resources);
        WebCore::String findAvailableBaseName(const WebCore::String& path, bool isTop);

};

FrameResource::FrameResource(WebCore::Frame* frame_, SerializeDomParam* param_)
	:frame(frame_), linkMap(param_->linkMap), serializerParam(param_),
	 mResourcePath(param_->resourcePath)
{
    createResourceDir(mResourcePath);
}

bool FrameResource::save()
{
	PassRefPtr<WebCore::HTMLCollection> images = frame->document()->images();
	bool imagesSaved = doSaveResources(images);

	PassRefPtr<WebCore::HTMLCollection> scripts = frame->document()->scripts();
	bool scriptsSaved = doSaveResources(scripts);

	PassRefPtr<WebCore::HTMLCollection> CSSs = frame->document()->CSSs();
	bool CSSsSaved = doSaveResources(CSSs);

	PassRefPtr<WebCore::HTMLCollection> embeds = frame->document()->embeds();
	bool embedsSaved = doSaveResources(embeds);

	PassRefPtr<WebCore::HTMLCollection> objects = frame->document()->objects();
	bool objectsSaved = doSaveResources(objects);

	PassRefPtr<WebCore::HTMLCollection> miscs = frame->document()->miscResources();
	bool miscsSaved = doSaveResources(miscs);

	return (imagesSaved && scriptsSaved && CSSsSaved && embedsSaved && objectsSaved && miscsSaved);
}

bool FrameResource::doSaveResources( PassRefPtr<WebCore::HTMLCollection> resources)
{
    int numResources = resources->length();
	//LOGSPE("Frame::doSaveResources numRes = %d", numResources);

    for (int i = 0; serializerParam->mErrorCode == 0 && i < numResources; i++) {
		serializerParam->webview->NotifyOneResourceDone();

        WebCore::Element* element = (WebCore::Element*)resources->item(i);
		if(element == NULL)
			continue;

        //For DRM contents, we need do something special later

        WebCore::String resourceURL;
        if (element->hasAttribute("src"))
            resourceURL = element->getAttribute("src");
        else if (element->hasAttribute("href"))
            resourceURL = element->getAttribute("href");
        else if (element->hasAttribute("data"))
            resourceURL = element->getAttribute("data");
        else if (element->hasAttribute("background"))
            resourceURL = element->getAttribute("background");

        if (resourceURL.length() == 0) {
            continue;
        }

		//LOGSPE("====>element=%s\n", element->toString().utf8().data());
		LOGSPE("resourceURL=%s  ", resourceURL.utf8().data());

        //generate local file name
        WebCore::String baseName = resourceURL;
        int pos = baseName.reverseFind('/');
        if (pos >= 0)
            baseName = baseName.substring(pos + 1);
        pos = baseName.reverseFind('?');
        if (pos >= 0)
            baseName.remove(pos, baseName.length() - pos);
        pos = baseName.reverseFind('.');

        WebCore::String extension;
        if (pos > 0) {
            extension = baseName.substring(pos + 1);
            baseName.remove(pos, baseName.length() - pos);
        }

        filterSpecialChars(baseName);
        if (baseName.length() > 32)
            baseName.truncate(32); //keep name short

        if (extension.length() > 4) { //looks not like a valid extension
            extension = WebCore::String();
        }
		LOGSPE("resource extension=%s\n", extension.utf8().data());

        if (extension.isEmpty()) {
            if (element->hasAttribute("type")) {
                extension = element->getAttribute("type");
            }
            else if (element->hasTagName(WebCore::HTMLNames::scriptTag)) {
                extension = "js";
            }
            else if (element->hasTagName(WebCore::HTMLNames::linkTag)) {
                extension = "css";
            }
            if (extension.isEmpty()) {
                //need to check stuff, like mime type?
                LOGSPE("ext:got nothing, make it to jpg?\n");
                LOGSPE("URL=%s\n", frame->document()->completeURL(resourceURL).string().utf8().data());
                extension = "jpg";
				//continue;
            }
        }

        modifyFileNameIfExists(mResourcePath, baseName, extension);

        //save file
        WebCore::DocLoader* loader = frame->document()->docLoader();
        WebCore::String completeURL = frame->document()->completeURL(resourceURL);
        if (loader && loader->cachedResource(completeURL) && !linkMap.contains(completeURL)) {
            WebCore::SharedBuffer* buffer = loader->cachedResource(completeURL)->data();

			if(buffer == NULL){
			    LOGSPE("buffer=%p ", buffer);
				continue;
			}

            const char* cachedData = buffer->data();
            int length = buffer->size();

            WebCore::String _fullPath = mResourcePath + baseName + "." + extension;
            WebCore::String fullPath = convertFileName(_fullPath);
            LOGSPE("...%d[%p:%d]->%s\n", i, cachedData, length, fullPath.utf8().data());
            FILE* fp = fopen(fullPath.utf8().data(), "w");
            if (fp) {
                int ret = fwrite(cachedData, 1, length, fp);
                //fputc('\0', fp);  //necessary to put eof?
                fclose(fp);

                WebCore::String rel_path;
                if(WebCore::equalIgnoringFragmentIdentifier(serializerParam->webview->mainFrame()->loader()->url(), frame->loader()->url()) == true){
                    LOGSPE("1111 fullPath=%s, rel_path=%s\n", fullPath.utf8().data(), rel_path.utf8().data());
                    rel_path = fullPath.replace(serializerParam->saveRootPath + "/", "");
                }
                else{
                    WebCore::String replaceString = mResourcePath;
                    rel_path = fullPath.replace(replaceString, "");
                    LOGSPE("2222 fullPath=%s, rel_path=%s\n", fullPath.utf8().data(), rel_path.utf8().data());
                }

                LOGSPE("set reslink=%s ==> %s\n", completeURL.utf8().data(), rel_path.utf8().data());
                linkMap.set(completeURL, rel_path);
            }
        }
        else {
            LOGSPE("****loader=%d, cached=%d, contained=%d, URL=%s\n", loader,
                   (loader)? loader->cachedResource(completeURL) : 0,
                   linkMap.contains(completeURL), completeURL.utf8().data());
        }
    }

	/*if( mCancelSaving == true){
		LOGSPE("Saving process was canceled by user\n");
		return false;
	}*/

    return true;
}

WebCore::String FrameResource::findAvailableBaseName(const WebCore::String& path, bool isTop)
{
    return getAvailableBaseName(frame, path, isTop);
}

class DomSerializeResult : public DomSerializerDelegate {
	public:
		DomSerializeResult(const WebCore::KURL& mainUrl, const WebCore::String& savePath,
				const WebCore::String& pagename, LinkLocalPathMap &linkmap)
			:mMainUrl(mainUrl),
			 mSavePath(savePath),
			 mPagename(pagename),
	         mLinkMap(linkmap){
			}

		int DidSerializeDataForFrame(
				const WebCore::Frame* frame,
				const char* data,
				PageSavingSerializationStatus status)
		{
			if(data == 0)
				return 0;

			LOGSPE("In DidSerializeDataForFrame, frame_url=%s, status=%d\n",
					frame->loader()->url().string().utf8().data(), status);

			WebCore::String saveFile;
			WTF::HashMap<const WebCore::Frame*, WebCore::String>::iterator it = mSavedFrames.find(frame);
			if (it != mSavedFrames.end()) {
				saveFile = it->second;
			}
			else{
				if(WebCore::equalIgnoringFragmentIdentifier(mMainUrl, frame->loader()->url()) == true){
					saveFile = mSavePath + "/" + mPagename + ".html";
				}
				else{
					WebCore::String savePath = mSavePath + "/" + mPagename + "_files/";
					WebCore::String htmlFileName = getAvailableBaseName(frame, savePath, false) + ".html";
					WebCore::String fullFileName = savePath + htmlFileName;
					saveFile = fullFileName;
				}
				mSavedFrames.set(frame, saveFile);
			}

            WebCore::String _saveFile = convertFileName(saveFile);
			LOGSPE("DidSerializeDataForFrame saveFile=%s\n", _saveFile.utf8().data());
			FILE *f = fopen(_saveFile.utf8().data(), "a");
            if(f == NULL){
                LOGSPE("Can't open file %s\n", saveFile.utf8().data());
                return -1;
            }

			if(fprintf(f, "%s", data) < 0){
                fclose(f);
                return -1;
            }
			fclose(f);

			if(status == CURRENT_FRAME_IS_FINISHED){
				WTF::HashMap<const WebCore::Frame*, WebCore::String>::iterator it = mSavedFrames.find(frame);
				if(it != mSavedFrames.end()) {
					WebCore::String saveFile = it->second;
					WebCore::String rel_path;
                    //if(equalIgnoringFragmentIdentifier(mMainUrl, frame->loader()->url()) == true){
                    //    LOGSPE("111 saveFile=%s, mSavePath=%s\n", saveFile.utf8().data(), mSavePath.utf8().data());
                        rel_path = saveFile.replace(mSavePath + "/", "");
                        rel_path = convertFileName(rel_path); //when write to main Html files ,also need to convertfilename.
                    //}
                    //else{
                    //    WebCore::String replaceString = mSavePath + "/" + mPagename + "_files/";
                    //    LOGSPE("222 saveFile=%s, mSavePath=%s\n", saveFile.utf8().data(), replaceString.utf8().data());
                    //    rel_path = saveFile.replace(replaceString, "");
                    //}
					LOGSPE("DidSerializeDataForFrame set content link [%s] ==> [%s]\n",
							frame->loader()->url().string().utf8().data(), rel_path.utf8().data());

                    if(frame->loader()->url().string().length() != 0){
                        mLinkMap.set(frame->loader()->url().string(), rel_path);
                    }
                    else{
                        return -1;
                    }
				}
			}

            return 0;
		}

	public:
		WebCore::KURL mMainUrl;
		const WebCore::String mSavePath;
		const WebCore::String mPagename;
		LinkLocalPathMap& mLinkMap;
		WTF::HashMap<const WebCore::Frame*, WebCore::String>  mSavedFrames;
};


bool SaveFrame(WebCore::Frame *specified_webframeimpl_,
        SerializeDomParam &param, DomSerializeResult *delegate)
{
    WTF::Vector<WebCore::Frame*> subframes;

    LOGSPE("specified_webframeimpl_=%p", specified_webframeimpl_);

    CollectSubFrames(specified_webframeimpl_, subframes);

    for(int i=0; param.mErrorCode == 0 && i < static_cast<int>(subframes.size()); ++i) {
        // Begin Motorola, krnf78, 04/20/2011, IKSTABLEFOURV-5669 / CLONE -[C_FIT]:CN:I1:F:N3:B1:Fail to save page.
        // Sub-frames without url should not be saved.
        if (subframes[i]->loader()->url().string().length() != 0) {
        // END IKSTABLEFOURV-5669
            SaveFrame(subframes[i], param, delegate);
        }
    }

	if(param.mErrorCode == 0){
		FrameResource frameResource(specified_webframeimpl_, &param);
		frameResource.save();

#if 1//SAVEPAGE_BY_DOM_SERIALIZE
		DomSerializer domSerializer(specified_webframeimpl_, param);
		domSerializer.SerializeDom();
#else
        //const String &saveCache = specified_webframeimpl_->loader()->getSaveCache();
        String saveCache(specified_webframeimpl_->loader()->getSaveCache());
        WTF::HashMap<WebCore::String, WebCore::String>::iterator it = param.linkMap.begin();
        while(it != param.linkMap.end()) {
            saveCache.replace(it->first, it->second);
            ++it;
        }

        WebCore::String saveFile;
        if(WebCore::equalIgnoringFragmentIdentifier(delegate->mMainUrl, specified_webframeimpl_->loader()->url()) == true){
            saveFile = delegate->mSavePath + "/" + delegate->mPagename + ".html";
        }
        else{
            WebCore::String savePath = delegate->mSavePath + "/" + delegate->mPagename + "_files/";
            WebCore::String htmlFileName = getAvailableBaseName(specified_webframeimpl_, savePath, false) + ".html";
            WebCore::String fullFileName = savePath + htmlFileName;
            saveFile = fullFileName;
            WebCore::String rel_path = saveFile.replace(delegate->mSavePath + "/", "");
            LOGSPE("set content link %s ==> %s\n", specified_webframeimpl_->loader()->url().string().utf8().data(), rel_path.utf8().data());
            delegate->mLinkMap.set(specified_webframeimpl_->loader()->url().string(), rel_path);
        }

        LOGSPE("saveFile=%s\n", saveFile.utf8().data());
        FILE *f = fopen(saveFile.utf8().data(), "a");
        if(f == NULL){
            LOGSPE("Can't open file %s\n", saveFile.utf8().data());
            param.mErrorCode = -1;
            return false;
        }

        fprintf(f, "%s", saveCache.utf8().data());
        fclose(f);
#endif

        return true;
	}

	return false;
}

static SerializeDomParam *g_domSerializerParam = 0;

int SaveFrame(WebCore::Frame *current_frame,
		const WebCore::String& savePath,
		WebCore::String& pagename,
		android::WebViewCore *webview){

    if(g_domSerializerParam != 0){
        return -4;
    }

    //FIXME: I suppose the pagename is not including path seperator, like \ or /
    pagename.replace('/', '_');

	filterSpecialChars(pagename);
    modifyFileNameIfExists(savePath, pagename, "html");
    LOGSPE("==>savePath=%s,savename=%s\n", savePath.utf8().data(), pagename.utf8().data());

	WebCore::String resPath = savePath + "/" + pagename + "_files/";
    createResourceDir(resPath);

	LinkLocalPathMap linkMap;

	DomSerializeResult delegate(current_frame->loader()->url(), savePath, pagename, linkMap);

	g_domSerializerParam = new SerializeDomParam(linkMap, &delegate, savePath, resPath, webview);

	SaveFrame(current_frame, *g_domSerializerParam, &delegate);
	// We have done call frames, so we send message to embedder to tell it that
	// frames are finished serializing.
	delegate.DidSerializeDataForFrame(current_frame, 0,
			DomSerializerDelegate::ALL_FRAMES_ARE_FINISHED);

    int result = g_domSerializerParam->mErrorCode;
    LOGSPE("++++++++++++==>SaveFrame return=%d param=%p\n", result, g_domSerializerParam);

	delete g_domSerializerParam;
	g_domSerializerParam = 0;

	return result;
}

void setCancelSaving(){
    LOGSPE("setCancelSaving %p\n", g_domSerializerParam);
	if(g_domSerializerParam != 0){
        LOGSPE("setCancelSaving param=%p\n", g_domSerializerParam);
		g_domSerializerParam->mErrorCode = -3;
	}
}

int getResourceNum(WebCore::Frame* frame){
	int numResources = 0;

	numResources += frame->document()->images()->length();
	numResources += frame->document()->scripts()->length();
	numResources += frame->document()->CSSs()->length();
	numResources += frame->document()->embeds()->length();
	numResources += frame->document()->objects()->length();
	numResources += frame->document()->miscResources()->length();

	for (Frame* child = frame->tree()->firstChild(); child; child = child->tree()->nextSibling()) {
		numResources += child->getResourceNum();
	}

	return numResources;
}

int getTotalFrameNum(WebCore::Frame* frame){
      WTF::Vector<WebCore::Frame*> frames;

      CollectTargetFrames(frame, frames);
      return static_cast<int>(frames.size());
}

}
