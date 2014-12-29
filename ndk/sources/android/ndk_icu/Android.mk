LOCAL_PATH := $(call my-dir)

#
# libndk_icu.a
#
include $(CLEAR_VARS)
LOCAL_MODULE := ndk_icu
LOCAL_CFLAGS := -I$(LOCAL_PATH)/include
LOCAL_SRC_FILES := ndk_icu.c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(BUILD_STATIC_LIBRARY)

