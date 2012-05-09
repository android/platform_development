
LOCAL_PATH:= $(call my-dir)
#
# static library for testing profiler
#
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_ENABLE_APROF:= true
LOCAL_SRC_FILES := libprof-test.cpp
LOCAL_MODULE := libprof-test
LOCAL_MODULE_TAGS := eng debug
include $(BUILD_STATIC_LIBRARY)

#
# shared ibrary for testing profiler
#
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
#LOCAL_ENABLE_APROF_JNI:= true
LOCAL_ENABLE_APROF:= true
#LOCAL_WHOLE_STATIC_LIBRARIES := libaprof-jni
#LOCAL_SHARED_LIBRARIES := libaprof libaprof-stubs libc
LOCAL_SRC_FILES := libprof-test.cpp
LOCAL_MODULE := libprof-test
#LOCAL_SHARED_LIBRARIES := libc
LOCAL_MODULE_TAGS := eng debug
include $(BUILD_SHARED_LIBRARY)

#
# Test profiler with shared libraries
#
include $(CLEAR_VARS)
LOCAL_ENABLE_APROF:= true
LOCAL_SRC_FILES := profiler-test.c
LOCAL_MODULE := profiler-test
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_SHARED_LIBRARIES := libprof-test libc
LOCAL_MODULE_TAGS := eng debug
include $(BUILD_EXECUTABLE)

#
# Test profiler with static link
#
include $(CLEAR_VARS)
LOCAL_ENABLE_APROF:= true
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_SRC_FILES := profiler-test.c
LOCAL_MODULE := profiler-test-static
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_STATIC_LIBRARIES := libprof-test libc
LOCAL_MODULE_TAGS := eng debug
include $(BUILD_EXECUTABLE)

#
# Test profiler with shared libraries which opened by dlopen
#
include $(CLEAR_VARS)
LOCAL_ENABLE_APROF:= true
LOCAL_SRC_FILES := profiler-test-dl.c
LOCAL_MODULE := profiler-test-dl
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_SHARED_LIBRARIES := libc libdl
LOCAL_MODULE_TAGS := eng debug
include $(BUILD_EXECUTABLE)

#
# Test profiling disabled executable link with profiling enabled libraries
#
include $(CLEAR_VARS)
LOCAL_SRC_FILES := profiler-test.c
LOCAL_MODULE := profiler-test-np
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_SHARED_LIBRARIES := libprof-test libc
LOCAL_MODULE_TAGS := eng debug
include $(BUILD_EXECUTABLE)
