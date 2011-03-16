# Build libqemud tests, included from main Android.mk

# The first test program is a simple TCP server that will send back
# anything it receives from the client.
#
include $(CLEAR_VARS)
LOCAL_MODULE := test-libqemud-host-1
LOCAL_SRC_FILES := test_host_1.c
LOCAL_MODULE_TAGS := debug
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test-libqemud-host-2
LOCAL_SRC_FILES := test_host_2.c
LOCAL_MODULE_TAGS := debug
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test-libqemud-guest-1
LOCAL_SRC_FILES := test_guest_1.c test_util.c
LOCAL_MODULE_TAGS := debug
LOCAL_STATIC_LIBRARIES := libqemud libcutils
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test-libqemud-guest-2
LOCAL_SRC_FILES := test_guest_2.c test_util.c
LOCAL_MODULE_TAGS := debug
LOCAL_STATIC_LIBRARIES := libqemud libcutils
include $(BUILD_EXECUTABLE)
