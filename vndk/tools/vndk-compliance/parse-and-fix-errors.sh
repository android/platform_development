#!/bin/bash
# This script uses log file saved from make >log 2>&1. It then parses and
# fixes the "file not found" errors by adding dependencies in reported modules'
# Android.mk file. It works for following path types:
# hardware/
# system/
# cutils/
# utils/
#
# More can be added.
#
# This script will create temp files log.<type> and log.<type>.names
# This script requires manual intervention in 2 places:
# 1. Visually inspecting log.<type>.names and removing undesirable lines
# 2. Manually checking in uncommitted files reported by repo status
#

if [ "$PWD" != "$ANDROID_BUILD_TOP" ]; then
  echo "This script needs to be run at top level folder"
  exit 1
fi
if [ ! -f "log" ]; then
  echo "log file not found"
  exit 1
fi

echo "Parsing log"
cat log | grep "FAILED\|error:" > log.error
echo "Parsing log.error for hardware"
cat log.error | grep -B1 "error: 'hardware\/" | grep FAILED | awk 'BEGIN{FS="_int"}{print $1}' | awk 'BEGIN{FS="S/";}{print $2}' | sort | uniq > log.hardware
echo "Parsing log.error for system"
cat log.error | grep -B1 "error: 'system\/" | grep FAILED | awk 'BEGIN{FS="_int"}{print $1}' | awk 'BEGIN{FS="S/";}{print $2}' | sort | uniq> log.system
echo "Parsing log.error for cutils"
cat log.error | grep -B1 "error: 'cutils\/" | grep FAILED | awk 'BEGIN{FS="_int"}{print $1}' | awk 'BEGIN{FS="S/";}{print $2}' | sort | uniq > log.cutils
echo "Parsing log.error for utils"
cat log.error | grep -B1 "error: 'utils\/" | grep FAILED | awk 'BEGIN{FS="_int"}{print $1}' | awk 'BEGIN{FS="S/";}{print $2}' | sort | uniq> log.utils

echo "Parsing log.hardware"
for i in `cat log.hardware`; do find . -name Android.\* | xargs grep -w -H $i | grep "LOCAL_MODULE\|name:"; done > log.hardware.names
echo "Parsing log.system"
for i in `cat log.system`; do find . -name Android.\* | xargs grep -w -H $i | grep "LOCAL_MODULE\|name:"; done > log.system.names
echo "Parsing log.cutils"
for i in `cat log.cutils`; do find . -name Android.\* | xargs grep -w -H $i | grep "LOCAL_MODULE\|name:"; done > log.cutils.names
echo "Parsing log.utils"
for i in `cat log.utils`; do find . -name Android.\* | xargs grep -w -H $i | grep "LOCAL_MODULE\|name:"; done > log.utils.names

echo "Please inspect log.*.names and remove lines with non-device related folders. Then press Enter"
read enter

cat log.hardware.names | awk 'BEGIN{FS=":"}{print $1}' | xargs sed -i '/include \$(BUILD/i LOCAL_HEADER_LIBRARIES += libhardware_headers'
echo "Checking for unsaved files"
repo status
echo "Please COMMIT them, then press Enter:"
read enter
cat log.system.names | awk 'BEGIN{FS=":"}{print $1}' | xargs sed -i '/include \$(BUILD/i LOCAL_HEADER_LIBRARIES += libsystem_headers'
echo "Checking for unsaved files"
repo status
echo "Please COMMIT them, then press Enter:"
cat log.cutils.names | awk 'BEGIN{FS=":"}{print $1}' | xargs sed -i '/include \$(BUILD/i LOCAL_SHARED_LIBRARIES += libcutils'
echo "Checking for unsaved files"
repo status
echo "Please COMMIT them, then press Enter:"
read enter
cat log.utils.names | awk 'BEGIN{FS=":"}{print $1}' | xargs sed -i '/include \$(BUILD/i LOCAL_SHARED_LIBRARIES += libutils'
echo "Checking for unsaved files"
repo status
echo "Please COMMIT them, then press Enter:"

