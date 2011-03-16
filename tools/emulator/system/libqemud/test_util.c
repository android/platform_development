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

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include "test_util.h"

#ifdef __linux__
#include <time.h>
double now_secs(void)
{
    struct timespec  tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return (double)tm.tv_sec + (double)tm.tv_nsec/1e9;
}
#else
#include <sys/time.h>
double now_secs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.sec + (double)tv.usec/1e6;
}
#endif

int
pipe_openSocket( Pipe*  pipe, int port )
{
    static int           fd;
    struct sockaddr_in  addr;

    pipe->pipe   = NULL;
    pipe->socket = -1;

    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if (fd < 0) {
        fprintf(stderr, "Can't create socket!!\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(0x0a000202);

    if ( connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ) {
        fprintf(stderr, "Can't connect to tcp:local:%d: %s\n", port,
                strerror(errno));
        close(fd);
        return -1;
    }

    pipe->socket = fd;
    return 0;
}

int
pipe_openQemudPipe( Pipe*  pipe, const char* pipename )
{
    pipe->pipe   = NULL;
    pipe->socket = -1;

    pipe->pipe = qemud_pipe_open(pipename);
    if (pipe->pipe == NULL) {
        fprintf(stderr, "Could not open '%s' pipe: %s\n", pipename, strerror(errno));
        return -1;
    }
    return 0;
}

int
pipe_send( Pipe*  pipe, const void* buff, size_t  bufflen )
{
    if (pipe->pipe != NULL)
        return qemud_pipe_send(pipe->pipe, buff, bufflen);
    else {
        int ret;
        const uint8_t* ptr = buff;
        while (bufflen > 0) {
            ret = send(pipe->socket, ptr, bufflen, 0);
            if (ret < 0) {
                if (errno == EINTR)
                    continue;
                fprintf(stderr, "%s: error: %s\n", __FUNCTION__, strerror(errno));
                return -1;
            }
            if (ret == 0) {
                fprintf(stderr, "%s: disconnection!\n", __FUNCTION__);
                return -1;
            }
            ptr     += ret;
            bufflen -= ret;
        }
        return 0;
    }
}

int
pipe_recv( Pipe*  pipe, void* buff, size_t bufflen )
{
    int  ret;

    if (pipe->pipe != NULL) {
        int  len2;
        ret = qemud_pipe_recv(pipe->pipe, buff, bufflen, &len2);
        if (!ret)
            ret = len2;

        return ret;
    }

    for (;;) {
        ret = recv(pipe->socket, buff, bufflen, 0);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "%s: error: %s\n", __FUNCTION__, strerror(errno));
            return -1;
        }
        if (ret == 0) {
            fprintf(stderr, "%s: disconnection!\n", __FUNCTION__);
            return -1;
        }
        break;
    }
    return ret;
}

void
pipe_close( Pipe*  pipe )
{
    if (pipe->pipe != NULL) {
        qemud_pipe_close(pipe->pipe);
        pipe->pipe = NULL;
    }
    if (pipe->socket >= 0) {
        close(pipe->socket);
        pipe->socket = -1;
    }
}
