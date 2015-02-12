#ifndef TextureManager_HPP
#define TextureManager_HPP

#include "PresetFrameIO.h"
#ifdef LINUX
#ifdef PROJECTM_ANDROID
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#ifdef WIN32
#include "glew.h"
#endif

#ifdef __APPLE__
#include <GL/gl.h>
#endif

#ifdef USE_DEVIL
#include <IL/ilut.h>
#else
#include "SOIL.h"
#endif

#include <iostream>
#include <string>
#include <map>

class TextureManager
{
  std::string presetURL;
  std::map<std::string,GLuint> textures;
public:
  ~TextureManager();
  TextureManager(std::string _presetURL);
  void unloadTextures(const PresetOutputs::cshape_container &shapes);
  void Clear();
  void Preload();
  GLuint getTexture(std::string imageUrl);
  unsigned int getTextureMemorySize();
};

#endif
