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
#ifndef _NATIVE_WINDOWING_H
#define _NATIVE_WINDOWING_H

#include <EGL/egl.h>

class NativeWindowing {
public:
    typedef enum {
        INPUT_EVENT_KEY_DOWN,
        INPUT_EVENT_KEY_UP,
        INPUT_EVENT_MOUSE_DOWN,
        INPUT_EVENT_MOUSE_UP,
        INPUT_EVENT_MOUSE_MOTION,  /* only with button down */
    } InputEventType;

    typedef enum {
        INPUT_KEY_BACK = 1000,
        INPUT_KEY_HOME,
        INPUT_KEY_MENU,
        INPUT_KEY_DPAD_LEFT,
        INPUT_KEY_DPAD_RIGHT,
        INPUT_KEY_DPAD_UP,
        INPUT_KEY_DPAD_DOWN,
        INPUT_KEY_VOLUME_UP,
        INPUT_KEY_VOLUME_DOWN,
        INPUT_KEY_CALL,
        INPUT_KEY_END_CALL,
        INPUT_KEY_ENTER,
    } InputKey;

    typedef struct {
        InputEventType  itype;
        union {
            /* for mouse events */
            struct {
                int      pos_x;
                int      pos_y;
                unsigned button_mask;
            };
            struct {
                int      key_code;
                int      key_unicode;
            };
        };
    } InputEvent;

    virtual NativeDisplayType getNativeDisplay() = 0;
    virtual NativeWindowType createNativeWindow(NativeDisplayType dpy, int width, int height) = 0;
    virtual int destroyNativeWindow(NativeDisplayType dpy, NativeWindowType win) = 0;
    virtual int pollEvent(NativeDisplayType dpy, NativeWindowType win, InputEvent& ev ) = 0;
};

#endif
