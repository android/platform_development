# Copyright 2007 The Android Open Source Project
#
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_JAVA_RESOURCE_DIRS := resources

LOCAL_JAVA_LIBRARIES := \
	ddmlib \
	swt \
	jcommon-1.0.12 \
	jfreechart-1.0.9 \
	jfreechart-1.0.9-swt

ifeq ($(SYSNAME),FreeBSD)
    LOCAL_JAVA_LIBRARIES += \
        org.eclipse.core.commands_3.3.0.I20070605-0010 \
        org.eclipse.equinox.common_3.3.0.v20070426 \
        org.eclipse.jface_3.3.2.M20080207-0800
else
    LOCAL_JAVA_LIBRARIES += \
        org.eclipse.jface_3.2.0.I20060605-1400 \
        org.eclipse.equinox.common_3.2.0.v20060603 \
        org.eclipse.core.commands_3.2.0.I20060605-1400
endif
								
LOCAL_MODULE := ddmuilib

include $(BUILD_HOST_JAVA_LIBRARY)

