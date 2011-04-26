#ifndef _SYSTEM_COMMON_EGL_CLIENT_IFACE_H
#define _SYSTEM_COMMON_EGL_CLIENT_IFACE_H

struct EGLThreadInfo;  // defined in ThreadInfo.h

typedef struct {
    EGLThreadInfo* (*getThreadInfo)();
} EGLClient_eglInterface;

typedef struct {
    void* (*getProcAddress)(const char *funcName);
} EGLClient_glesInterface;

//
// Any GLES/GLES2 client API library should define a function named "init_emul_gles"
// with the following prototype,
// It will be called by EGL after loading the GLES library for initialization
// and exchanging interface function pointers.
//
typedef EGLClient_glesInterface *(*init_emul_gles_t)(EGLClient_eglInterface *eglIface);

#endif
