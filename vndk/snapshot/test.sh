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

# Usage: After creating the VNDK snapshots with ./build.sh, run this script

set -e

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ANDROID_BUILD_TOP=$(dirname $(dirname $(dirname $script_dir)))
echo "ANDROID_BUILD_TOP is $ANDROID_BUILD_TOP"

DIST_DIR=$ANDROID_BUILD_TOP/out/dist
SNAPSHOT_TOP=$DIST_DIR/vndk-snapshot

snapshot_arm64=$DIST_DIR/android-vndk-arm64.zip
snapshot_x86_64=$DIST_DIR/android-vndk-x86_64.zip

SNAPSHOT_TEMPFILE=$DIST_DIR/snapshot_snapshot.txt
INSTALL_TEMPFILE=$DIST_DIR/snapshot_install.txt

RED='\033[0;31m'
NC='\033[0m'
TEST_PASS_MSG="===== Result: PASS ====="
TEST_FAIL_MSG="${RED}===== Result: FAIL =====${NC}"

echo 'Removing any old $DIST_DIR/vndk-snapshot/*'
rm -rf $SNAPSHOT_TOP

echo "Unzipping $snapshot_arm64"
unzip $snapshot_arm64 -d $DIST_DIR 1>/dev/null

echo "Unzipping $snapshot_x86_64"
unzip $snapshot_x86_64 -d $DIST_DIR 1>/dev/null

# $1: vndk_type: string, one of [vndk-core, vndk-sp]
# $2: arch: string, one of [arm, arm64, x86, x86_64]
compare_files() {
    local vndk_type=$1
    local arch=$2
    local product=
    local bitness=
    local snapshot_dir=
    local install_vndk_dir=
    local install_dir=

    if [[ ${arch:0:3} =~ 'arm' ]]; then
        product='generic_arm64_ab'
    else
        product='generic_x86_64'
    fi

    if [[ ${arch:-2:length} =~ '64' ]]; then
        bitness='64'
    else
        bitness=''
    fi

    snapshot_dir=$SNAPSHOT_TOP/arch-$arch/shared/$vndk_type

    if [[ $vndk_type =~ 'vndk-core' ]]; then
        install_vndk_dir='vndk'
    else
        install_vndk_dir='vndk-sp'
    fi

    install_dir=$ANDROID_BUILD_TOP/out/target/product/$product/system/lib$bitness/$install_vndk_dir

    ls -1 $snapshot_dir > $SNAPSHOT_TEMPFILE
    find $install_dir -type f | xargs -n 1 -I file bash -c "basename file" | sort > $INSTALL_TEMPFILE

    echo "Diffing snapshot against installed files for VNDK=$vndk_type and ARCH=$arch"
    diff $SNAPSHOT_TEMPFILE $INSTALL_TEMPFILE && echo $TEST_PASS_MSG || echo -e $TEST_FAIL_MSG
}

ARCHS=('arm' 'arm64' 'x86' 'x86_64')

for arch in "${ARCHS[@]}"; do
    compare_files 'vndk-core' $arch
    compare_files 'vndk-sp' $arch
done

echo "Removing temp files..."
rm -rf $SNAPSHOT_TEMPFILE $INSTALL_TEMPFILE

echo "Finished all testcases"
