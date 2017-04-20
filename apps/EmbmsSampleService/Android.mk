LOCAL_PATH:= $(call my-dir)

# Build the Sample Embms Services
include $(CLEAR_VARS)

src_dirs := src
res_dirs := res

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_dirs))
LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(res_dirs))

LOCAL_AAPT_FLAGS := \
        --auto-add-overlay \

LOCAL_PACKAGE_NAME := SampleEmbmsService

LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true

include $(BUILD_PACKAGE)


# Build the test package
include $(call all-makefiles-under,$(LOCAL_PATH))

