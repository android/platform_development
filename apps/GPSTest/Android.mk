LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_MODULE_TAGS := eng development

# Maps support is disabled
# LOCAL_JAVA_LIBRARIES := com.google.android.maps

LOCAL_PACKAGE_NAME := GPSTest
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)
