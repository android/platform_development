#ifndef _SYSTEM_EGL_DISPLAY_H
#define _SYSTEM_EGL_DISPLAY_H

#include <pthread.h>
#include "glUtils.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "EGLClientIface.h"
#include <utils/KeyedVector.h>

#define ATTRIBUTE_NONE -1
//FIXME: are we in this namespace?
using namespace android;

class eglDisplay
{
public:
    eglDisplay();

    bool initialize(EGLClient_eglInterface *eglIface);
    void terminate();

    int getVersionMajor() const { return m_major; }
    int getVersionMinor() const { return m_minor; }
    bool initialized() const { return m_initialized; }

    const char *queryString(EGLint name);

    const EGLClient_glesInterface *gles_iface() const { return m_gles_iface; }
    const EGLClient_glesInterface *gles2_iface() const { return m_gles2_iface; }

	int 	getNumConfigs(){ return m_numConfigs; }
	EGLBoolean  getConfigAttrib(EGLConfig config, EGLint attrib, EGLint * value);
	EGLBoolean getConfigPixelFormat(EGLConfig config, GLenum * format);

private:
    EGLClient_glesInterface *loadGLESClientAPI(const char *libName, 
                                               EGLClient_eglInterface *eglIface);
	EGLBoolean getAttribValue(EGLConfig config, EGLint attribIdxi, EGLint * value);

private:
    pthread_mutex_t m_lock;
    bool m_initialized;
    int  m_major;
    int  m_minor;
    int  m_hostRendererVersion;
    int  m_numConfigs;
    int  m_numConfigAttribs;

	/* This is the mapping between an attribute name to it's index in any given config */
	DefaultKeyedVector<EGLint, EGLint>	m_attribs;
	/* This is an array of all config's attributes values stored in the following sequencial fasion (read: v[c,a] = the value of attribute <a> of config <c>)
	 * v[0,0],..,v[0,m_numConfigAttribs-1],
	 *...
	 * v[m_numConfigs-1,0],..,v[m_numConfigs-1,m_numConfigAttribs-1]
	 */
    EGLint *m_configs;
    EGLClient_glesInterface *m_gles_iface;
    EGLClient_glesInterface *m_gles2_iface;
    const char *m_versionString;
    const char *m_vendorString;
    const char *m_extensionString;
};

#endif
