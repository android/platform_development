LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=  \
        egl.cpp \
        egl_dispatch.cpp

LOCAL_MODULE := libEGL_host_wrapper
LOCAL_MODULE_TAGS := debug

OS_LDLIBS :=
ifeq ($(HOST_OS),linux)
    OS_LDLIBS := -ldl
endif

LOCAL_LDLIBS := $(OS_LDLIBS)

include $(BUILD_HOST_SHARED_LIBRARY) 
