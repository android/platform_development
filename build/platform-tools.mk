##############################################################################
# Platform Tools Component
#
# Inputs:
#   my_prefix: HOST_ or HOST_CROSS_
##############################################################################

LOCAL_NAME := platform-tools
LOCAL_PREFIX_DIR := platform-tools
LOCAL_FILES :=
LOCAL_STRIP_FILES :=
LOCAL_DIRS :=

LOCAL_FILES += \
  development/sdk/sdk_files_NOTICE.txt:NOTICE.txt \
  $($(my_prefix)OUT)/development/sdk/plat_tools_source.properties:source.properties

LOCAL_FILES += \
  $($(my_prefix)OUT_EXECUTABLES)/adb$($(my_prefix)EXECUTABLE_SUFFIX):adb$($(my_prefix)EXECUTABLE_SUFFIX) \
  $($(my_prefix)OUT_EXECUTABLES)/fastboot$($(my_prefix)EXECUTABLE_SUFFIX):fastboot$($(my_prefix)EXECUTABLE_SUFFIX) \
  $($(my_prefix)OUT_EXECUTABLES)/mke2fs$($(my_prefix)EXECUTABLE_SUFFIX):mke2fs$($(my_prefix)EXECUTABLE_SUFFIX) \
  $($(my_prefix)OUT_EXECUTABLES)/mke2fs.conf:mke2fs.conf \
  $($(my_prefix)OUT_EXECUTABLES)/make_f2fs$($(my_prefix)EXECUTABLE_SUFFIX):make_f2fs$($(my_prefix)EXECUTABLE_SUFFIX) \
  $($(my_prefix)OUT_EXECUTABLES)/make_f2fs_casefold$($(my_prefix)EXECUTABLE_SUFFIX):make_f2fs_casefold$($(my_prefix)EXECUTABLE_SUFFIX) \
  $($(my_prefix)OUT_EXECUTABLES)/sqlite3$($(my_prefix)EXECUTABLE_SUFFIX):sqlite3$($(my_prefix)EXECUTABLE_SUFFIX) \
  $($(my_prefix)OUT_EXECUTABLES)/dmtracedump$($(my_prefix)EXECUTABLE_SUFFIX):dmtracedump$($(my_prefix)EXECUTABLE_SUFFIX) \
  $($(my_prefix)OUT_EXECUTABLES)/etc1tool$($(my_prefix)EXECUTABLE_SUFFIX):etc1tool$($(my_prefix)EXECUTABLE_SUFFIX) \
  $($(my_prefix)OUT_EXECUTABLES)/hprof-conv$($(my_prefix)EXECUTABLE_SUFFIX):hprof-conv$($(my_prefix)EXECUTABLE_SUFFIX) \

ifneq ($($(my_prefix)OS),windows)
LOCAL_FILES += \
  $($(my_prefix)OUT_EXECUTABLES)/e2fsdroid:e2fsdroid \
  $($(my_prefix)OUT_EXECUTABLES)/sload_f2fs:sload_f2fs \
  $($(my_prefix)OUT_SHARED_LIBRARIES)/libc++$($(my_prefix)SHLIB_SUFFIX):lib64/libc++$($(my_prefix)SHLIB_SUFFIX)
else # Windows
LOCAL_FILES += \
  $($(my_prefix)OUT_SHARED_LIBRARIES)/AdbWinUsbApi.dll:lib/AdbWinUsbApi.dll \
  $($(my_prefix)OUT_SHARED_LIBRARIES)/AdbWinApi.dll:lib/AdbWinApi.dll \
  prebuilts/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/x86_64-w64-mingw32/lib32/libwinpthread-1.dll:libwinpthread-1.dll
endif

# systrace
LOCAL_DIRS += \
  external/chromium-trace/catapult:systrace/catapult
LOCAL_FILES += \
  external/chromium-trace/systrace.py:systrace/systrace.py \
  external/chromium-trace/NOTICE:systrace/NOTICE \
  external/chromium-trace/UPSTREAM_REVISION:systrace/UPSTREAM_REVISION

include $(LOCAL_PATH)/sdk_repo.mk
