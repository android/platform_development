#Howto make your build VNDK compliant

##Enable VNDK flag
In your device's BoardConfig.mk, set BOARD_VNDK_VERSION := current
Set that device's lunch combo and make

##Fixing build errors
The resulting errors will be mainly of 2 types
###Copy headers not allowed
Vendor modules are not allowed to use LOCAL_COPY_HEADERS. They need to export
their headers. This is an example on how to do that:
1. Lets call the guilty module libabc. Open libabc's Android.mk
2. Note all the headers that are being copied by libabc
3. Create a local dir called include (or inc). Add symlinks to every file that is
   being copied. If all the files are in the same folder, the include dir itself
   will be a symlink to that folder
4. In Android.mk, remove all lines with LOCAL_COPY_HEADERS_TO and LOCAL_COPY_HEADERS
5. Replace these lines with 
   LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := libabc_headers
6. Create the module_headers lib outside the definition of current module
  include (CLEAR_VARS)
  LOCAL_MODULE := libabc_headers
  LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
  include $(BUILD_HEADER_LIBRARY)
7. Fix all such copy header violations.

###Headers not found
1. Once all copy header violations are cleaned up, make will start throwing lots of
   "file not found" errors. These are due to 2 reasons:
   a. Modules relying on copy headers are not finding those headers anymore due
   to above changes
   b. VNDK build rules remove global includes from the path. So dirs like
   system/core/include, frameworks/av/include or hardware/libhardware/include
   will no longer be offered in include path
2. Fix them using the parse_and_fix_errors.sh script. Customize it according to
   your needs.
  


