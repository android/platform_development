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

exe :=
ifeq ($($(my_prefix)OS),windows)
  exe := .exe
endif

LOCAL_FILES += \
  $($(my_prefix)OUT_EXECUTABLES)/adb$(exe) \
  $($(my_prefix)OUT_EXECUTABLES)/fastboot$(exe) \
  $($(my_prefix)OUT_EXECUTABLES)/mke2fs$(exe) \
  $($(my_prefix)OUT_EXECUTABLES)/make_f2fs$(exe) \
  $($(my_prefix)OUT_EXECUTABLES)/make_f2fs_casefold$(exe) \
  $($(my_prefix)OUT_EXECUTABLES)/sqlite3$(exe) \
  $($(my_prefix)OUT_EXECUTABLES)/dmtracedump$(exe) \
  $($(my_prefix)OUT_EXECUTABLES)/etc1tool$(exe) \
  $($(my_prefix)OUT_EXECUTABLES)/hprof-conv$(exe)

# TODO: was this in the windows builds before, if it was, where was it coming from?
ifneq ($($(my_prefix)OS),windows)
LOCAL_FILES += \
  $($(my_prefix)OUT_EXECUTABLES)/mke2fs.conf
endif

ifneq ($($(my_prefix)OS),windows)
LOCAL_FILES += \
  $($(my_prefix)OUT_EXECUTABLES)/e2fsdroid \
  $($(my_prefix)OUT_EXECUTABLES)/sload_f2fs \
  $($(my_prefix)OUT_SHARED_LIBRARIES)/libc++$($(my_prefix)SHLIB_SUFFIX):lib64/libc++$($(my_prefix)SHLIB_SUFFIX)
else # Windows
LOCAL_FILES += \
  $($(my_prefix)OUT_SHARED_LIBRARIES)/AdbWinUsbApi.dll:lib/AdbWinUsbApi.dll \
  $($(my_prefix)OUT_SHARED_LIBRARIES)/AdbWinApi.dll:lib/AdbWinApi.dll \
  prebuilts/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/x86_64-w64-mingw32/lib32/libwinpthread-1.dll
endif

# systrace
LOCAL_DIRS += \
  external/chromium-trace/catapult:systrace/catapult
LOCAL_FILES += \
  external/chromium-trace/systrace.py:systrace/systrace.py \
  external/chromium-trace/NOTICE:systrace/NOTICE \
  external/chromium-trace/UPSTREAM_REVISION:systrace/UPSTREAM_REVISION

exe :=

include $(LOCAL_PATH)/sdk_repo.mk
