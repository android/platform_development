
LOCAL_PATH:=$(call my-dir)

# ut_renderer test program ###########################

include $(CLEAR_VARS)

ifeq ($(HOST_OS), linux)

emulatorOpengl := $(LOCAL_PATH)/../..

LOCAL_MODULE := ut_renderer
LOCAL_MODULE_TAGS := debug
LOCAL_ADDITIONAL_DEPENDENCIES := $(ALL_GENERATED_SOURCES)

LOCAL_SRC_FILES := ut_renderer.cpp \
        RenderingThread.cpp \
	ReadBuffer.cpp \
	Renderer.cpp \
	RendererContext.cpp \
	RendererSurface.cpp \
	X11Windowing.cpp \
    TimeUtils.cpp

LOCAL_CFLAGS := -DPVR_WAR 
#LOCAL_CFLAGS += -g -O0

LOCAL_C_INCLUDES := $(emulatorOpengl)/system/OpenglCodecCommon \
		$(call intermediates-dir-for, SHARED_LIBRARIES, libut_rendercontrol_dec, HOST) \
		$(call intermediates-dir-for, SHARED_LIBRARIES, libGLESv1_dec, HOST) \
        $(emulatorOpengl)/host/libs/GLESv1_dec \
        $(emulatorOpengl)/system/GLESv1_enc \
        $(emulatorOpengl)/tests/ut_rendercontrol_enc

LOCAL_SHARED_LIBRARIES := libut_rendercontrol_dec libGLESv1_dec libEGL_host_wrapper
LOCAL_STATIC_LIBRARIES := libOpenglCodecCommon
LOCAL_LDLIBS := -lpthread -lX11 -lrt
include $(BUILD_HOST_EXECUTABLE)

endif # HOST_OS == linux
