#include "eglDisplay.h"
#include "HostConnection.h"
#include <dlfcn.h>

const int systemEGLVersionMajor = 1;
const int systemEGLVersionMinor = 4;
const char *systemEGLVendor = "Google Android emulator";

// list of extensions supported by this EGL implementation
const char *systemStaticEGLExtensions = 
            "";

// list of extensions supported by this EGL implementation only if supported
// on the host implementation.
const char *systemDynamicEGLExtensions = 
            "";


eglDisplay::eglDisplay() :
    m_lock(PTHREAD_MUTEX_INITIALIZER),
    m_initialized(false),
    m_major(0),
    m_minor(0),
    m_hostRendererVersion(0),
    m_numConfigs(0),
    m_numConfigAttribs(0),
	m_attribs(DefaultKeyedVector<EGLint, EGLint>(ATTRIBUTE_NONE)),
    m_configs(NULL),
    m_gles_iface(NULL),
    m_gles2_iface(NULL),
    m_versionString(NULL),
    m_vendorString(NULL),
    m_extensionString(NULL)
{
}

bool eglDisplay::initialize(EGLClient_eglInterface *eglIface)
{
    pthread_mutex_lock(&m_lock);
    if (!m_initialized) {

        //
        // load GLES client API
        //
        m_gles_iface = loadGLESClientAPI("/system/lib/egl/libGLESv1_CM_emulation.so", 
                                         eglIface);
        if (!m_gles_iface) {
            return false;
        }

#ifdef WITH_GLES2
        m_gles2_iface = loadGLESClientAPI("/system/lib/egl/libGLESv2_emulation.so", 
                                          eglIface);
#endif

        //
        // establish connection with the host
        //
        HostConnection *hcon = HostConnection::get();
        if (!hcon) {
            return false;
        }

        //
        // get renderControl encoder instance
        //
        renderControl_encoder_context_t *rcEnc = hcon->rcEncoder();
        if (!rcEnc) {
            return false;
        }

        //
        // Query host reneder and EGL version
        //
        m_hostRendererVersion = rcEnc->rcGetRendererVersion(rcEnc);
        EGLint status = rcEnc->rcGetEGLVersion(rcEnc, &m_major, &m_minor);
        if (status != EGL_TRUE) {
            // host EGL initialization failed !!
            return false;
        }

        //
        // Take minimum version beween what we support and what the host support
        //
        if (m_major > systemEGLVersionMajor) {
            m_major = systemEGLVersionMajor;
            m_minor = systemEGLVersionMinor;
        }
        else if (m_major == systemEGLVersionMajor &&
                 m_minor > systemEGLVersionMinor) {
            m_minor = systemEGLVersionMinor;
        }

        //
        // Query the host for the set of configs
        //
        m_numConfigs = rcEnc->rcGetNumConfigs(rcEnc, (uint32_t*)&m_numConfigAttribs);
        if (m_numConfigs <= 0 || m_numConfigAttribs <= 0) {
            // just sanity check - should never happen
            return false;
        }

        uint32_t nInts = m_numConfigAttribs * (m_numConfigs + 1);
		EGLint tmp_buf[nInts];
        m_configs = new EGLint[nInts-1];
        if (!m_configs) {
            return false;
        }

        //EGLint n = rcEnc->rcGetConfigs(rcEnc, nInts*sizeof(EGLint), m_configs);
        EGLint n = rcEnc->rcGetConfigs(rcEnc, nInts*sizeof(EGLint), (GLuint*)tmp_buf);
        if (n != m_numConfigs) {
            return false;
        }

		//Fill the attributes vector.
		//The first m_numConfigAttribs values of tmp_buf are the actual attributes enums.
		for (int i=0; i<m_numConfigAttribs; i++)
		{
			m_attribs.add(tmp_buf[i], i);
		}

		//Copy the actual configs data to m_configs
		memcpy(m_configs, tmp_buf + m_numConfigAttribs*sizeof(EGLint), m_numConfigs*sizeof(EGLint));

        m_initialized = true;
    }
    pthread_mutex_unlock(&m_lock);

    return true;
}

void eglDisplay::terminate()
{
    pthread_mutex_lock(&m_lock);
    if (m_initialized) {
        m_initialized = false;
        delete m_configs;
        m_configs = NULL;
    }
    pthread_mutex_unlock(&m_lock);
}

EGLClient_glesInterface *eglDisplay::loadGLESClientAPI(const char *libName, 
                                                       EGLClient_eglInterface *eglIface)
{
    void *lib = dlopen(libName, RTLD_NOW);
    if (!lib) {
        return NULL;
    }

    init_emul_gles_t init_gles_func = (init_emul_gles_t)dlsym(lib,"init_emul_gles");
    if (!init_gles_func) {
        dlclose((void*)libName);
        return NULL;
    }

    return (*init_gles_func)(eglIface);
}

static char *queryHostEGLString(EGLint name)
{
    HostConnection *hcon = HostConnection::get();
    if (hcon) {
        renderControl_encoder_context_t *rcEnc = hcon->rcEncoder();
        if (rcEnc) {
            int n = rcEnc->rcQueryEGLString(rcEnc, name, NULL, 0);
            if (n < 0) {
                char *str = (char *)malloc(-n+1);
                n = rcEnc->rcQueryEGLString(rcEnc, name, str, -n);
                if (n > 0) {
                    return str;
                }

                free(str);
            }
        }
    }

    return NULL;
}

static char *buildExtensionString()
{
    //Query host extension string
    const char *hostExt = queryHostEGLString(EGL_EXTENSIONS);
    if (!hostExt || (hostExt[0] == '\0')) {
        // no extensions on host - only static extension list supported
        return (char*)systemStaticEGLExtensions;
    }

    //
    // Filter host extension list to include only extensions
    // we can support (in the systemDynamicEGLExtensions list)
    //
    char *ext = (char *)hostExt;
    char *c = ext;
    char *insert = ext;
    while(c) {
        if (*c == ' ' || *c == '\0') {
            bool done = (*c == '\0');
            if (!done) {
                c++;
            }
            char *save = c;
            *c = '\0';
            if (strstr(ext, systemDynamicEGLExtensions)) {
                if (ext != insert) {
                    strcpy(insert, ext);
                }
                insert += strlen(ext);
            }
            c = save;

            if (done) {
                break;
            }
        }
        c++;
    }
    *insert = '\0';
  
    int n = strlen(hostExt);
    if (n > 0) {
        char *str = (char *)malloc(strlen(systemStaticEGLExtensions) + n + 1);
        snprintf(str,n,"%s%s", systemStaticEGLExtensions, hostExt);
        free((char*)hostExt);
        return str;
    }
    else {
        free((char*)hostExt);
        return (char*)systemStaticEGLExtensions;
    }
}

const char *eglDisplay::queryString(EGLint name)
{
    if (name == EGL_CLIENT_APIS) {
        return "OpenGL_ES";
    }
    else if (name == EGL_VERSION) {
        if (m_versionString) {
            return m_versionString;
        }

        // build version string
        pthread_mutex_lock(&m_lock);
        char str[12];
        snprintf(str, 12, "%d.%d", m_major, m_minor);
        m_versionString = strdup(str);
        pthread_mutex_unlock(&m_lock);

        return m_versionString;
    }
    else if (name == EGL_VENDOR) {
        if (m_vendorString) {
            return m_vendorString;
        }

        // build vendor string
        const char *hostVendor = queryHostEGLString(EGL_VENDOR);

        pthread_mutex_lock(&m_lock);
        if (hostVendor) {
            int n = strlen(systemEGLVendor) + 7 + strlen(hostVendor) + 1;
            char *str = (char *)malloc( strlen(systemEGLVendor) + 7 + strlen(hostVendor) );
            snprintf(str, n, "%s Host: %s", systemEGLVendor, hostVendor);
            m_vendorString = str;
            free((char*)hostVendor);
        }
        else {
            m_vendorString = systemEGLVendor;
        }
        pthread_mutex_unlock(&m_lock);

        return m_vendorString;
    }
    else if (name == EGL_EXTENSIONS) {
        if (m_extensionString) {
            return m_extensionString;
        }

        // build extension string
        pthread_mutex_lock(&m_lock);
        m_extensionString = buildExtensionString();
        pthread_mutex_unlock(&m_lock);

        return m_extensionString;
    }
	else {
		LOGE("[%s] Unknown name %d\n", __FUNCTION__, name);
		return NULL;
	}
}

EGLBoolean eglDisplay::getAttribValue(EGLConfig config, EGLint attribIdx, EGLint * value)
{
	if (attribIdx == ATTRIBUTE_NONE)
	{
		LOGE("[%s] Bad attribute idx\n", __FUNCTION__);
		return EGL_FALSE;
	}
	*value = *(m_configs + (int)config*m_numConfigAttribs + attribIdx);
	return EGL_TRUE;
}

EGLBoolean eglDisplay::getConfigAttrib(EGLConfig config, EGLint attrib, EGLint * value)
{
	return getAttribValue(config, m_attribs.valueFor(attrib), value);  
}

EGLBoolean eglDisplay::getConfigPixelFormat(EGLConfig config, GLenum * format)
{
	EGLint redSize, blueSize, greenSize, alphaSize;
	
	if ( !(getAttribValue(config, m_attribs.valueFor(EGL_RED_SIZE), &redSize) &&
		getAttribValue(config, m_attribs.valueFor(EGL_BLUE_SIZE), &blueSize) &&
		getAttribValue(config, m_attribs.valueFor(EGL_GREEN_SIZE), &greenSize) &&
		getAttribValue(config, m_attribs.valueFor(EGL_ALPHA_SIZE), &alphaSize)) )
	{
		LOGE("Couldn't find value for one of the pixel format attributes");
		return EGL_FALSE;
	}

	//calculate the GL internal format
	if ((redSize==8)&&(blueSize==8)&&(greenSize==8)&&(alphaSize==8)) *format = GL_RGBA;
	else if ((redSize==8)&&(blueSize==8)&&(greenSize==8)&&(alphaSize==0)) *format = GL_RGB; 
	else if ((redSize==5)&&(blueSize==6)&&(greenSize==5)&&(alphaSize==0)) *format = GL_RGB565_OES;
	else if ((redSize==5)&&(blueSize==5)&&(greenSize==5)&&(alphaSize==1)) *format = GL_RGB5_A1_OES;
	else if ((redSize==4)&&(blueSize==4)&&(greenSize==4)&&(alphaSize==4)) *format = GL_RGBA4_OES;
	else return EGL_FALSE;

	return EGL_TRUE;
}
