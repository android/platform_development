#!/bin/bash
#
# Copyright (C) 2018 The Android Open Source Project
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
#   usage: live-image-boot.sh [system.img]
#
#       when used w/o the 1st argument, fetch the image list from GCP for user
#       to select.

# name of the GCP storage bucket

run_command() {
    local cmd="$1"
    local message="$2"
    echo ${message}
    (${cmd})
}
source ${0%/*}/live-image-utils.sh

readonly IMAGE_LIST=${IMAGE_LIST:-gsi.html}
readonly CACHE_DIR=${CACHE_DIR:-${HOME}/Downloads}
# BUCKET is defined in the live-image-utils.sh
readonly BUCKET_URI="https://storage.googleapis.com/${BUCKET}"

check_required_utils "wget gunzip adb simg2img" || exit 1
mkdir -p ${CACHE_DIR}
file="$1"
if [ -z $file ]; then
    wget ${BUCKET_URI}/${IMAGE_LIST} -O ${CACHE_DIR}/${IMAGE_LIST} >/dev/null 2>&1
    n=0
    while read l ; do

        # e.g. The line format is like below
        # q.aosp_arm64_ab-userdebug.20181030.raw.gz 1443135488 Android GSI/AOSP
        file=`echo $l | cut -d ' ' -f1`
        title=`echo $l | cut -d ' ' -f3,4,5,6`
        echo [$n] $title
        eval "s${n}=$file"
        n=$(($n+1))
    done < ${CACHE_DIR}/${IMAGE_LIST}
    file=""
    while [ -z $file ]; do
        echo -n "select?"
        read select
        eval "file=\$s$select"
    done
    url=${BUCKET_URI}/$file
    cache=${CACHE_DIR}/${file}
    echo ${cache}
    raw=${file%.*}
    if ! [ -f ${cache} ]; then
        run_command "wget $url -O ${CACHE_DIR}/${file}" "fetch..."
        run_command "gunzip -c ${CACHE_DIR}/${file} > ${raw}" "gzip..."
    fi
else
    raw=${file%.*}.raw
    run_command "simg2img $file ${raw}" "simg2img $file ${raw}"
fi
adb root
n=`adb shell cat /sys/class/zram-control/hot_add`
echo "using zram$n"
adb shell "echo 8G > /sys/block/zram$n/disksize"
adb push ${raw} /dev/block/zram$n
adb shell sync
run_command "adb shell \"vdc --wait checkpoint startCheckpoint 1\"" "# start checking point"
echo -n "# Enter to reboot the device" && read ans
adb reboot
