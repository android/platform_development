/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Set to 1 to enable debugging */
#define DEBUG  0

#if DEBUG >= 1
#  define D(...)  fprintf(stderr,"libqemud:" __VA_ARGS__), fprintf(stderr, "\n")
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <hardware/qemud.h>
#include <hardware/qemud_pipe.h>
#include <pthread.h>  /* for pthread_once() */
#include <stdlib.h>
#include <stdio.h>

/* Used for debugging */

#ifndef D
#  define  D(...)   do{}while(0)
#endif

struct QemudPipe {
    int pipe_fd;
};

/* Try to open a qemud pipe, 'pipeName' must be a generic pipe service
 * name (e.g. "opengles" or "camera"). The emulator will be in charge of
 * connecting the corresponding pipe/client to an internal service or an
 * external socket, these details are hidden from the caller.
 *
 * Return a new QemudPipe pointer, or NULL in case of error
 */
QemudPipe*
qemud_pipe_open(const char*  pipeName)
{
    int   pipeNameLen;
    int   fd, ret;

    if (pipeName == NULL || pipeName[0] == '\0') {
        errno = EINVAL;
        return NULL;
    }

    fd = open("/dev/qemu_pipe", O_RDWR);
    if (fd < 0) {
        D("Could not open /dev/qemu_pipe: %s", strerror(errno));
        return NULL;
    }

    pipeNameLen = strlen(pipeName);

    ret = TEMP_FAILURE_RETRY(write(fd, pipeName, pipeNameLen+1));
    if (ret != pipeNameLen+1) {
        D("Could not connect to %s pipe service: %s", pipeName,
          strerror(errno));
        return NULL;
    }

    QemudPipe* pipe = malloc(sizeof(*pipe));

    if (pipe == NULL) {
        D("Not enough memory to allocate new QemudPipe object!");
        close(fd);
        return NULL;
    }

    pipe->pipe_fd = fd;
    return pipe;
}

/* Send a message through a QEMUD pipe. This is a blocking call.
 *
 * On success, this returns 0, and _all_ bytes of the message will have been
 * transfered to the emulator (partial sends are not supported).
 *
 * On failure, this returns -1, and errno can take one of the following
 * values:
 *
 *    EINVAL      -> bad QemudPipe handle, or bufferSize is < 0.
 *    ECONNRESET  -> connection to the pipe has been reset/lost.
 *    ENOMEM      -> message is too large
 *
 * The ECONNRESET error typically occurs when the pipe was closed by the
 * service/process behind it, or when a remote pipe could not connect to
 * a remote process after a specific timeout.
 *
 * The ENOMEM error only occurs when the message is so large that there isn't
 * any memory in to allocate a buffer to receive the message. I
 */
int
qemud_pipe_send(QemudPipe* pipe, const void* buffer, int bufferSize)
{
    const char*  p;
    int len = 0;

    if (pipe == NULL || pipe->pipe_fd < 0 || bufferSize < 0) {
        errno = EINVAL;
        return -1;
    }

    if (bufferSize == 0) {
        return 0;
    }

    p = buffer;
    while (bufferSize > 0) {
        int ret = write(pipe->pipe_fd, p, bufferSize);
        if (ret > 0) {
            p += ret;
            len += ret;
            bufferSize -= ret;
            continue;
        }
        if (ret == 0) {
            break;
        }
        if (errno == EINTR)
            continue;

        if (len == 0)
            len = -1;

        break;
    }
    return len;
}
/* Receive a message from a QEMUD pipe. This is a blocking call.
 *
 * On success, this returns the number of bytes in the incoming message, which
 * will be <= bufferSize. If bufferSize is 0, this returns immediately with 0.
 *
 * On failure, this returns -1 and errno can take one of the following values:
 *
 *    EINVAL        -> bad QemudPipe handle, or bufferSize is < 0
 *    ECONNRESET    -> connection to the pipe has been reset/losst
 *    ENOMEM        -> 'bufferSize' is too small to receive the message.
 *
 * When ENOMEM is returned, the message is still waiting
 */
int
qemud_pipe_recv(QemudPipe* pipe, void* buffer, int bufferSize, int *msgSize)
{
    int  len = 0;
    char* p = buffer;

    if (pipe == NULL || pipe->pipe_fd < 0 || bufferSize < 0) {
        errno = EINVAL;
        return -1;
    }

    if (bufferSize == 0) {
        *msgSize = 0;
        return 0;
    }

    for (;;) {
        int ret = read(pipe->pipe_fd, p, bufferSize);
        if (ret >= 0) {
            len = ret;
            break;
        }
        if (errno == EINTR)
            continue;

        return -1;
    }

    *msgSize = len;
    return 0;
}

void
qemud_pipe_close(QemudPipe*  pipe)
{
    if (pipe != NULL && pipe->pipe_fd >= 0) {
        close(pipe->pipe_fd);
        pipe->pipe_fd = -1;
        free(pipe);
    }
}
