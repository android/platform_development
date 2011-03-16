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

/* This program benchmarks a QEMUD pipe to exchange data with a test
 * server.
 *
 * See test_host_1.c for the corresponding server code, which simply
 * sends back anything it receives from the client.
 */
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "test_util.h"

#define  PIPE_NAME  "pingpong"

int main(void)
{
    Pipe        pipe[1];
    int         port = 8012;
    int         maxCount   = 1000;
    int         bufferSize = 16384;
    uint8_t*    buffer;
    uint8_t*    buffer2;
    int         nn, count;
    double      time0, time1;

#if 0
    if (pipe_openSocket(pipe, port) < 0) {
        fprintf(stderr, "Could not open tcp socket!\n");
        return 1;
    }
    printf("Connected to tcp:host:%d\n", port);
#else
    if (pipe_openQemudPipe(pipe, PIPE_NAME) < 0) {
        fprintf(stderr, "Could not open '%s' pipe: %s\n", PIPE_NAME, strerror(errno));
        return 1;
    }
    printf("Connected to '%s' pipe\n", PIPE_NAME);
#endif

    buffer  = malloc(bufferSize);
    buffer2 = malloc(bufferSize);

    for (nn = 0; nn < bufferSize; nn++) {
        buffer[nn] = (uint8_t)nn;
    }

    time0 = now_secs();

    for (count = 0; count < maxCount; count++) {
        int ret = pipe_send(pipe, buffer, bufferSize);
        int pos, len;

        if (ret < 0) {
            fprintf(stderr,"%d: Sending %d bytes failed: %s\n", count, bufferSize, strerror(errno));
            return 1;
        }

#if 1
        /* The server is supposed to send the message back */
        pos = 0;
        len = bufferSize;
        while (len > 0) {
            ret = pipe_recv(pipe, buffer2 + pos, len);
            if (ret < 0) {
                fprintf(stderr, "Receiving failed (ret=%d): %s\n", ret, strerror(errno));
                return 3;
            }
            if (ret == 0) {
                fprintf(stderr, "Disconnection while receiving!\n");
                return 4;
            }
            pos += ret;
            len -= ret;
        }

        if (memcmp(buffer, buffer2, bufferSize) != 0) {
            fprintf(stderr, "Message content mismatch!\n");
            return 6;
        }

#endif
        if (count > 0 && (count % 100) == 0) {
            printf("... %d\n", count);
        }
    }

    time1 = now_secs();

    printf("Closing pipe\n");
    pipe_close(pipe);

    printf("Total time: %g seconds\n", time1 - time0);
    printf("Total bytes: %g bytes\n", 1.0*maxCount*bufferSize);
    printf("Bandwidth: %g MB/s\n", (maxCount*bufferSize/(1024.0*1024.0))/(time1 - time0) );
    return 0;
}
