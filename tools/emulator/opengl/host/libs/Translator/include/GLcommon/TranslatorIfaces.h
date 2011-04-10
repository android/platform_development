/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef TRANSLATOR_IFACES_H
#define TRANSLATOR_IFACES_H
#include <GLcommon/ThreadInfo.h>
#include <GLES/gl.h>

extern "C" {

class TextureData : public ObjectData
{
public:
    ~TextureData() {
        if (sourceEGLImage && eglImageDetach) (*eglImageDetach)(sourceEGLImage);
    }
    TextureData():width(0),height(0),border(0),internalFormat(GL_RGBA),sourceEGLImage(0){};

    unsigned int width;
    unsigned int height;
    unsigned int border;
    unsigned int internalFormat;
    unsigned int sourceEGLImage;
    void (*eglImageDetach)(unsigned int imageId);
};

struct EglImage
{
    ~EglImage(){};
    unsigned int imageId;
    unsigned int globalTexName;
    unsigned int width;
    unsigned int height;
    unsigned int internalFormat;
    unsigned int border;
};

typedef SmartPtr<EglImage> ImagePtr;
typedef  std::map< unsigned int, ImagePtr>       ImagesHndlMap;

class GLEScontext;

typedef struct {
    GLEScontext* (*createGLESContext)();
    void         (*initContext)(GLEScontext*);
    void         (*deleteGLESContext)(GLEScontext*);
    void         (*flush)();
    void         (*finish)();
    void         (*setShareGroup)(GLEScontext*,ShareGroupPtr);
}GLESiface;


typedef struct {
    ThreadInfo* (*getThreadInfo)();
    EglImage* (*eglAttachEGLImage)(unsigned int imageId);
    void        (*eglDetachEGLImage)(unsigned int imageId);
}EGLiface;

typedef GLESiface* (*__translator_getGLESIfaceFunc)(EGLiface*);

}
#endif
