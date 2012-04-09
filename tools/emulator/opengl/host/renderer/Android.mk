LOCAL_PATH:=$(call my-dir)

ifeq ($(HOST_OS),linux)
    $(call emugl-export,LDLIBS,-lX11)
endif

# host renderer process ###########################
$(call emugl-begin-host-executable,emulator_renderer)
$(call emugl-import,libOpenglRender)
LOCAL_SRC_FILES := main.cpp
LOCAL_CFLAGS    += -O0 -g

#ifeq ($(HOST_OS),windows)
#LOCAL_LDLIBS += -lws2_32
#endif

$(call emugl-end-module)

