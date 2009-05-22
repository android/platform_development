LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := SdkSetup
LOCAL_CERTIFICATE := platform

# To enable provisioning hack
LOCAL_MODULE_TAGS := eng development

include $(BUILD_PACKAGE)
