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
#include "emulator-console.h"
#include "sockets.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEBUG  0
#if DEBUG
#  define D(...)  printf(__VA_ARGS__), printf("\n")
#else
#  define D(...)  ((void)0)
#endif

#define ANEW0(p)  (p) = calloc(sizeof(*(p)), 1)

enum {
    STATE_CONNECTING = 0,
    STATE_CONNECTED,
    STATE_ERROR = 2
};

typedef struct Msg {
    const char*   data;  // pointer to data
    int           size;  // size of data
    int           sent;  // already sent (so sent..size remain in buffer).
    struct Msg*  next;  // next message in queue.
} Msg;

static Msg*
msg_alloc( const char* data, int  datalen )
{
    Msg*  msg;

    msg = malloc(sizeof(*msg) + datalen);
    msg->data = (const char*)(msg + 1);
    msg->size = datalen;
    msg->sent = 0;
    memcpy((char*)msg->data, data, datalen);
    msg->next = NULL;

    return msg;
}

static void
msg_free( Msg*  msg )
{
    free(msg);
}

struct EmulatorConsole {
    int        fd;
    IoLooper*  looper;
    int        state;
    Msg*       out_msg;
};

/* Read as much from the input as possible, ignoring it.
 */
static int
emulatorConsole_eatInput( EmulatorConsole* con )
{
    for (;;) {
        char temp[64];
        int ret = socket_recv(con->fd, temp, sizeof temp);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            }
            return -1;
        }
        if (ret == 0) {
            return -1;
        }
        D("Console received: '%.*s'", ret, temp);
    }
}

static int
emulatorConsole_sendOutput( EmulatorConsole* con )
{
    if (con->state != STATE_CONNECTED) {
        errno = EINVAL;
        return -1;
    }

    while (con->out_msg != NULL) {
        Msg* msg = con->out_msg;
        int  ret;

        ret = socket_send(con->fd,
                          msg->data + msg->sent,
                          msg->size - msg->sent);
        if (ret > 0) {
            D("Console sent: '%.*s'", ret, msg->data + msg->sent);

            msg->sent += ret;
            if (msg->sent == msg->size) {
                con->out_msg = msg->next;
                msg_free(msg);
            }
            continue;
        }
        if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }
        con->state = STATE_ERROR;
        D("Console error when sending: %s", strerror(errno));
        return -1;
    }
    iolooper_del_write(con->looper, con->fd);
    return 0;
}

static void
emulatorConsole_completeConnect(EmulatorConsole* con)
{
    D("Console connected!");
    iolooper_add_read(con->looper, con->fd);
    iolooper_del_write(con->looper, con->fd);
    con->state = STATE_CONNECTED;
    if (con->out_msg != NULL) {
        iolooper_add_write(con->looper, con->fd);
        emulatorConsole_sendOutput(con);
    }
}

/* Create a new EmulatorConsole object to connect asynchronously to
 * a given emulator port. Note that this should always succeeds since
 * the connection is asynchronous.
 */
EmulatorConsole*
emulatorConsole_connect(int port, IoLooper* looper)
{
    EmulatorConsole*  con;
    SockAddress  addr;

    ANEW0(con);
    con->looper = looper;
    con->fd = socket_create_inet( SOCKET_STREAM );
    if (con->fd < 0) {
        con->state = STATE_ERROR;
        return con;
    }

    socket_set_nonblock(con->fd);
    sock_address_init_inet(&addr, SOCK_ADDRESS_INET_LOOPBACK, port);
    if (socket_connect(con->fd, &addr) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
            con->state = STATE_CONNECTING;
            iolooper_add_write(con->looper, con->fd);
        } else {
            fprintf(stderr, "Cannot connect to emulator console on port %d: %s",
                   port, strerror(errno));
            con->state = STATE_ERROR;
        }
        return con;
    }

    emulatorConsole_completeConnect(con);
    return con;
}

int
emulatorConsole_poll( EmulatorConsole*  con )
{
    if (!iolooper_is_read(con->looper, con->fd) &&
        !iolooper_is_write(con->looper, con->fd))
    {
        return 0;
    }

LOOP:
    switch (con->state) {
        case STATE_ERROR:
            return -1;

        case STATE_CONNECTING:
            // read socket error to determine success / error.
            if (socket_get_error(con->fd) != 0) {
                goto SET_ERROR;
            }
            emulatorConsole_completeConnect(con);
            return 0;

        case STATE_CONNECTED:
            /* ignore input, if any */
            if (iolooper_is_read(con->looper, con->fd)) {
                if (emulatorConsole_eatInput(con) < 0) {
                    goto SET_ERROR;
                }
            }
            /* send outgoing data, if any */
            if (iolooper_is_write(con->looper, con->fd)) {
                if (emulatorConsole_sendOutput(con) < 0) {
                    goto SET_ERROR;
                }
            }
            return 0;
    }

SET_ERROR:
    D("Console ERROR!: %s\n", strerror(errno));
    con->state = STATE_ERROR;
    return -1;
}

/* Send a message to the console asynchronously. Any answer will be
 * ignored. */
void
emulatorConsole_send( EmulatorConsole*  con, const char* command )
{
    int  cmdlen = strlen(command);
    Msg* msg;
    Msg** plast;

    if (cmdlen == 0)
        return;

    D("%s: sending '%s'\n", __FUNCTION__, command);

    /* Append new message at end of outgoing list */
    msg = msg_alloc(command, cmdlen);
    plast = &con->out_msg;
    while (*plast) {
        plast = &(*plast)->next;
    }
    *plast = msg;
    if (con->out_msg == msg) {
        D("Console enabling writes!\n");
        iolooper_add_write(con->looper, con->fd);
    }
    emulatorConsole_sendOutput(con);
}


void
emulatorConsole_sendMouseDown( EmulatorConsole* con, int x, int y )
{
    char temp[128];

    snprintf(temp, sizeof temp,
             "event send 3:0:%d 3:1:%d 1:330:1 0:0:0\r\n",
             x, y);
    emulatorConsole_send(con, temp);
}

void
emulatorConsole_sendMouseMotion( EmulatorConsole* con, int x, int y )
{
    /* Same as mouse down */
    emulatorConsole_sendMouseDown(con, x, y);
}

void
emulatorConsole_sendMouseUp( EmulatorConsole* con, int x, int y )
{
    char temp[128];

    snprintf(temp, sizeof temp,
             "event send 3:0:%d 3:1:%d 1:330:0 0:0:0\r\n",
             x, y);
    emulatorConsole_send(con, temp);
}

#define EE(x,y)  if (keycode == x) return y;

static int convertKeycode(int keycode)
{
    /* Keycode values expected by the Linux kernel, and the emulator */
    enum {
        KEY_BACK = 158,
        KEY_HOME = 102,
        KEY_SOFT1 = 229,
        KEY_LEFT = 105,
        KEY_UP   = 103,
        KEY_DOWN =  108,
        KEY_RIGHT = 106,
        KEY_VOLUMEUP = 115,
        KEY_VOLUMEDOWN = 114,
        KEY_SEND = 231,
        KEY_END = 107,
        KEY_ENTER = 28,
    };

    /* See input codes in NativeWindowing.h */
    EE(1000,KEY_BACK);
    EE(1001,KEY_HOME);
    EE(1002,KEY_SOFT1);
    EE(1003,KEY_LEFT);
    EE(1004,KEY_RIGHT);
    EE(1005,KEY_UP);
    EE(1006,KEY_DOWN);
    EE(1007,KEY_VOLUMEUP);
    EE(1008,KEY_VOLUMEDOWN);
    EE(1009,KEY_SEND);
    EE(1010,KEY_END);
    EE(1011,KEY_ENTER);

    return keycode;
}

void
emulatorConsole_sendKey( EmulatorConsole* con, int keycode, int down )
{
    char temp[128];

    snprintf(temp, sizeof temp,
             "event send EV_KEY:%d:%d 0:0:0\r\n", convertKeycode(keycode), down);
    emulatorConsole_send(con, temp);
}
