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
#include "X11Windowing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define DEBUG 0
#if DEBUG
#  define D(...) printf(__VA_ARGS__), printf("\n")
#else
#  define D(...) ((void)0)
#endif

/* Try to remember the window position between creates/destroys */
static int X11_wmXPos = 100;
static int X11_wmYPos = 100;

static int X11_wmXAdjust = 0;
static int X11_wmYAdjust = 0;

static void
get_window_pos( Display *disp, Window win, int *px, int *py )
{
    Window  child;

    XTranslateCoordinates( disp, win, DefaultRootWindow(disp),  0, 0, px, py, &child );
}


static void
set_window_pos(Display *disp, Window win, int x, int y)
{
    int   xNew, yNew;
    int   xAdjust = X11_wmXAdjust;
    int   yAdjust = X11_wmYAdjust;

    /* this code is tricky because some window managers, but not all,
     * will translate the final window position by a given offset
     * corresponding to the frame decoration.
     *
     * so we first try to move the window, get the position that the
     * window manager has set, and if they are different, re-position the
     * window again with an adjustment.
     *
     * this causes a slight flicker since the window 'jumps' very
     * quickly from one position to the other.
     */

    D("%s: move to [%d,%d] adjusted to [%d,%d]", __FUNCTION__,
      x, y, x+xAdjust, y+yAdjust);
    XMoveWindow(disp, win, x + xAdjust, y + yAdjust);
    XSync(disp, True);
    get_window_pos(disp, win, &xNew, &yNew);
    if (xNew != x || yNew != y) {
        X11_wmXAdjust = xAdjust = x - xNew;
        X11_wmYAdjust = yAdjust = y - yNew;
        D("%s: read pos [%d,%d], recomputing adjust=[%d,%d] moving to [%d,%d]\n",
          __FUNCTION__, xNew, yNew, xAdjust, yAdjust, x+xAdjust, y+yAdjust);
        XMoveWindow(disp, win, x + xAdjust, y + yAdjust );
    }
    XSync(disp, False);
}


NativeDisplayType X11Windowing::getNativeDisplay()
{
    if (mDpy == NULL) {
        Display *dpy = XOpenDisplay(NULL);
        mDpy = dpy;
    }
    return (NativeDisplayType)mDpy;
}

NativeWindowType X11Windowing::createNativeWindow(NativeDisplayType _dpy, int width, int height)
{
    Display *dpy = (Display *) _dpy;

    long defaultScreen = DefaultScreen( dpy );
    Window rootWindow = RootWindow(dpy, defaultScreen);
    int depth = DefaultDepth(dpy, defaultScreen);
    XVisualInfo *visualInfo = new XVisualInfo;

    XMatchVisualInfo(dpy, defaultScreen, depth, TrueColor, visualInfo);
    if (visualInfo == NULL) {
        fprintf(stderr, "couldn't find matching visual\n");
        return NULL;
    }

    Colormap x11Colormap = XCreateColormap(dpy, rootWindow, visualInfo->visual, AllocNone);
    XSetWindowAttributes sWA;
    sWA.colormap = x11Colormap;
    sWA.event_mask = StructureNotifyMask | ExposureMask;
    sWA.background_pixel = 0;
    sWA.border_pixel = 0;
    unsigned int attributes_mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

    Window win = XCreateWindow( dpy,
                                rootWindow,
                                X11_wmXPos, X11_wmYPos, width, height,
                                0, CopyFromParent, InputOutput,
                                CopyFromParent, attributes_mask, &sWA);

    XMapWindow(dpy, win);
    XSelectInput(dpy, win, ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask);
    XFlush(dpy);

    set_window_pos(dpy, win, X11_wmXPos, X11_wmYPos);

    mMousePressed = 0;
    mMouseLastX = 0;
    mMouseLastY = 0;

    return NativeWindowType(win);
}

int X11Windowing::destroyNativeWindow(NativeDisplayType _dpy, NativeWindowType _win)
{
    Display *dpy = (Display *)_dpy;
    Window win = (Window)_win;
    get_window_pos(dpy, win, &X11_wmXPos, &X11_wmYPos);
    D("%s: Saved window position [%d, %d]\n", __FUNCTION__, X11_wmXPos, X11_wmYPos);
    XDestroyWindow(dpy, win);
    XFlush(dpy);
    return 0;
}


#define SS_(x,y)  x ## y
#define SS(x,y)  SS_(x,y)
#define EE(x,y)  if (keycode == x) return SS(X11Windowing::INPUT_KEY_,y)

static int toInputKey(int keycode)
{
    /* Mapping for a normal PC US QWERTY keyboard */
    EE(9,BACK);
    EE(110,HOME);
    EE(68,MENU);
    EE(69,CALL);
    EE(70,END_CALL);
    EE(86,VOLUME_UP);
    EE(82,VOLUME_DOWN);
    EE(111,DPAD_UP);
    EE(113,DPAD_LEFT);
    EE(114,DPAD_RIGHT);
    EE(116,DPAD_DOWN);
    EE(36,ENTER);
    return keycode;
}

int X11Windowing::pollEvent(NativeDisplayType _dpy, NativeWindowType _win, InputEvent& ev )
{
    Display *dpy = (Display *)_dpy;
    Window win = (Window)_win;

    //printf("%s:%d\n", __FUNCTION__, __LINE__);
    /* First, we need to check if there is at least one event in the
     * input queue without blocking. Logic borrowed from the SDL library.
     */
    XFlush(dpy);
    if (!XEventsQueued(dpy, QueuedAlready)) {
        /* Is x11 ready to talk to us? */
        struct timeval zero;
        //printf("%s:%d\n", __FUNCTION__, __LINE__);
        zero.tv_sec = zero.tv_usec = 0;
        int fd = ConnectionNumber(dpy);
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);
        if (select(fd+1, &fdset, NULL, NULL, &zero) != 1) {
            //printf("%s:%d\n", __FUNCTION__, __LINE__);
            return 0;
        }
    }

    /* Ok, we have at least one event - grab it */
    //printf("%s:%d\n", __FUNCTION__, __LINE__);
    XEvent  xev;
    memset(&xev, 0, sizeof(xev));
    XNextEvent(dpy, &xev);

    /* Now convert this baby to something we understand */
    switch (xev.type) {
        case ButtonPress: {
            XButtonEvent* bev = (XButtonEvent*) &xev;
            ev.pos_x = mMouseLastX = bev->x;
            ev.pos_y = mMouseLastY = bev->y;
            mMousePressed = true;
            ev.itype = INPUT_EVENT_MOUSE_DOWN;
            return 1;
        }
        case ButtonRelease: {
            XButtonEvent* bev = (XButtonEvent*) &xev;
            ev.pos_x = mMouseLastX = bev->x;
            ev.pos_y = mMouseLastY = bev->y;
            mMousePressed = false;
            ev.itype = INPUT_EVENT_MOUSE_UP;
            return 1;
        }
        case MotionNotify:
            //printf("%s:%d\n", __FUNCTION__, __LINE__);
            if (mMousePressed) {
                //printf("%s:%d\n", __FUNCTION__, __LINE__);
                ev.itype = INPUT_EVENT_MOUSE_MOTION;
                ev.pos_x = mMouseLastX = ((XMotionEvent*)&xev)->x;
                ev.pos_y = mMouseLastY = ((XMotionEvent*)&xev)->y;;
                return 1;
            }
            break;

        case KeyPress: {
            XKeyEvent* kev = (XKeyEvent*) &xev;
            //printf("%s:%d PRESS keycode=%d\n", __FUNCTION__, __LINE__, kev->keycode);
            ev.itype = INPUT_EVENT_KEY_DOWN;
            ev.key_code = toInputKey(kev->keycode);
            return 1;
        }

        case KeyRelease: {
            XKeyEvent* kev = (XKeyEvent*) &xev;
            //printf("%s:%d RELEASE keycode=%d\n", __FUNCTION__, __LINE__, kev->keycode);
            ev.itype = INPUT_EVENT_KEY_UP;
            ev.key_code = toInputKey(kev->keycode);
            return 1;
        }

        default:
            printf("%s:%d\n", __FUNCTION__, __LINE__);
            // TODO: KEY EVENTS TO QWERTY
            ;
    }
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}
