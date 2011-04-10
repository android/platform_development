#ifndef __COMMON_HOST_CONNECTION_H
#define __COMMON_HOST_CONNECTION_H

#include "IOStream.h"
#include "GLEncoder.h"
#include "renderControl_enc.h"

class HostConnection
{
public:
    static HostConnection *get();
    ~HostConnection();

    GLEncoder *glEncoder();
    renderControl_encoder_context_t *rcEncoder();

    void flush() {
        if (m_stream) {
            m_stream->flush();
        }
    }

private:
    HostConnection();
    static gl_client_context_t *s_getGLContext();

private:
    IOStream *m_stream;
    GLEncoder *m_glEnc;
    renderControl_encoder_context_t *m_rcEnc;
};

#endif
