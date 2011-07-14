LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmonkeyprocess
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE)
LOCAL_SRC_FILES := com_android_commands_monkey_MonkeyProcess.cpp
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
