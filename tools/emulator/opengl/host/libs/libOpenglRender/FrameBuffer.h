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
#ifndef _LIBRENDER_FRAMEBUFFER_H
#define _LIBRENDER_FRAMEBUFFER_H

#include "libOpenglRender/render_api.h"
#include "ColorBuffer.h"
#include "RenderContext.h"
#include "WindowSurface.h"
#include <utils/threads.h>
#include <map>
#include <EGL/egl.h>
#include <stdint.h>

#if defined(__linux__) || defined(_WIN32) || defined(__VC32__) && !defined(__CYGWIN__)
#else
#warning "Unsupported Platform"
#endif

typedef uint32_t HandleType;
typedef std::map<HandleType, RenderContextPtr> RenderContextMap;
typedef std::map<HandleType, WindowSurfacePtr> WindowSurfaceMap;
typedef std::map<HandleType, ColorBufferPtr> ColorBufferMap;

struct FrameBufferCaps
{
    bool hasGL2;
    bool has_eglimage_texture_2d;
    bool has_eglimage_renderbuffer;
    bool has_BindToTexture;
    EGLint eglMajor;
    EGLint eglMinor;
};

class FrameBuffer
{
public:
    static bool initialize(FBNativeWindowType p_window,
                           int x, int y,
                           int width, int height);
    static FrameBuffer *getFB() { return s_theFrameBuffer; }

    const FrameBufferCaps &getCaps() const { return m_caps; }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    HandleType createRenderContext(int p_config, HandleType p_share, bool p_isGL2 = false);
    HandleType createWindowSurface(int p_config, int p_width, int p_height);
    HandleType createColorBuffer(int p_width, int p_height, GLenum p_internalFormat);
    void DestroyRenderContext(HandleType p_context);
    void DestroyWindowSurface(HandleType p_surface);
    void DestroyColorBuffer(HandleType p_colorbuffer);

    bool  bindContext(HandleType p_context, HandleType p_drawSurface, HandleType p_readSurface);
    bool  setWindowSurfaceColorBuffer(HandleType p_surface, HandleType p_colorbuffer);
    bool  flushWindowSurfaceColorBuffer(HandleType p_surface);
    bool  bindColorBufferToTexture(HandleType p_colorbuffer);
    bool updateColorBuffer(HandleType p_colorbuffer,
                           int x, int y, int width, int height,
                           GLenum format, GLenum type, void *pixels);

    bool post(HandleType p_colorbuffer);

    EGLDisplay getDisplay() const { return m_eglDisplay; }
    EGLContext getContext() const { return m_eglContext; }

    bool bind_locked();
    bool unbind_locked();

private:
    FrameBuffer(int p_x, int p_y, int p_width, int p_height);
    ~FrameBuffer();
    HandleType genHandle();

private:
    static FrameBuffer *s_theFrameBuffer;
    static HandleType s_nextHandle;
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    android::Mutex m_lock;
    FBNativeWindowType m_nativeWindow;
    FrameBufferCaps m_caps;
    EGLDisplay m_eglDisplay;
    RenderContextMap m_contexts;
    WindowSurfaceMap m_windows;
    ColorBufferMap m_colorbuffers;

    EGLSurface m_eglSurface;
    EGLContext m_eglContext;

    EGLContext m_prevContext;
    EGLSurface m_prevReadSurf;
    EGLSurface m_prevDrawSurf;
};
#endif
