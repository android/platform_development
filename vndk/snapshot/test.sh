#!/bin/bash
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This script is a test script for build/make/core/tasks/vndk.mk,
# making sure the files packaged in the VNDK snapshot are the same as the files
# installed under $(PRODUCT_OUT)/system/lib*
#
# Usage: After creating the VNDK snapshots with ./build.sh, run this script

set -euo pipefail

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 TARGET_ARCH"
    exit 1
fi

ARCH=$1

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ANDROID_BUILD_TOP=$(dirname $(dirname $(dirname $script_dir)))
echo "ANDROID_BUILD_TOP: $ANDROID_BUILD_TOP"

DIST_DIR=$ANDROID_BUILD_TOP/out/dist
SNAPSHOT_TOP=$DIST_DIR/android-vndk-snapshot
SNAPSHOT_TEMPFILE=$DIST_DIR/snapshot_libs.txt
SYSTEM_TEMPFILE=$DIST_DIR/system_libs.txt

RED='\033[0;31m'
NC='\033[0m'
TEST_PASS_MSG="===== Result: PASS ====="
TEST_FAIL_MSG="${RED}===== Result: FAIL =====${NC}"

echo 'Removing any old $DIST_DIR/android-vndk-snapshot/*'
rm -rf $SNAPSHOT_TOP

snapshot_zip=$DIST_DIR/android-vndk-$ARCH.zip
echo "Unzipping $snapshot_zip"
unzip $snapshot_zip -d $DIST_DIR &>/dev/null || (echo "No file: $snapshot_zip" && exit 1)

# $1: vndk_type: string, one of [vndk-core, vndk-sp]
# $2: arch: string, one of [arm, arm64, x86, x86_64]
compare_files() {
    local vndk_type=$1
    local arch=$2
    local product=
    local bitness=
    local snapshot_dir=
    local system_vndk_dir=
    local system_dir=

    if [[ $arch == 'arm64' ]]; then
        product='generic_arm64_ab'
    elif [[ $arch == 'arm' ]]; then
        product='generic'
    elif [[ $arch == 'x86_64' ]]; then
        product='generic_x86_64'
    elif [[ $arch == 'x86' ]]; then
        product='generic_x86'
    fi

    if [[ ${arch:-2:length} =~ '64' ]]; then
        bitness='64'
    else
        bitness=''
    fi

    snapshot_dir=$SNAPSHOT_TOP/arch-$arch*/shared/$vndk_type

    if [[ $vndk_type =~ 'vndk-core' ]]; then
        system_vndk_dir='vndk'
    else
        system_vndk_dir='vndk-sp'
    fi

    system_dir=$ANDROID_BUILD_TOP/out/target/product/$product/system/lib$bitness/$system_vndk_dir

    ls -1 $snapshot_dir > $SNAPSHOT_TEMPFILE
    find $system_dir -type f | xargs -n 1 -I file bash -c "basename file" | sort > $SYSTEM_TEMPFILE

    echo "Diffing snapshot against installed system libs for VNDK=$vndk_type and ARCH=$arch"
    (diff --old-line-format="Only found in VNDK snapshot: %L" --new-line-format="Only found in system/lib*: %L" \
      --unchanged-line-format="" $SNAPSHOT_TEMPFILE $SYSTEM_TEMPFILE && echo $TEST_PASS_MSG) \
    || (echo -e $TEST_FAIL_MSG; exit 1)
}


###########################################################
# TESTCASES

# TEST: Check VNDK-core and VNDK-SP files
echo "[Test] Checking file list for VNDK-core and VNDK-SP"
compare_files 'vndk-core' $ARCH
compare_files 'vndk-sp' $ARCH

# TEST: Check .txt files
echo "[Test] Listing config files"
if [ -d $SNAPSHOT_TOP/arch-$ARCH*/configs ]; then
    find $SNAPSHOT_TOP/arch-$ARCH*/configs -type f | grep -o "configs/.*";
else
    echo "Directory does not exist: $SNAPSHOT_TOP/arch-$ARCH*/configs"
fi;

# TEST: Check directory structure
echo "[Test] Checking directory structure of vndk-snapshot/"
find $SNAPSHOT_TOP -type d | grep -o "android-vndk-snapshot/arch-$ARCH.*"

echo "Finished all testcases"


###########################################################
# Clean up

echo "Removing temp files..."
rm -rf $SNAPSHOT_TEMPFILE $SYSTEM_TEMPFILE

echo "End of script"
