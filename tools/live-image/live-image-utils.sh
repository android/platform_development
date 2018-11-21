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

# name of the GCP project
PROJECT_ID=${PROJECT_ID:-treble-keystone}

# name of the GCP storage bucket
BUCKET=${BUCKET:-treble-gsi}

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "The \$ANDROID_BUILD_TOP is not set, lunch first."
    return
fi

check_required_utils() {
    local REQUIRED_EXTERNAL_UTILS="$1"
    for util in ${REQUIRED_EXTERNAL_UTILS}; do
        type ${util} >/dev/null 2>&1 || "${util} not found."
    done
}

gcp_login()
{
    check_required_utils "gcloud"
    gcloud config set project "${PROJECT_ID}"
    gcloud auth login
}

# extract the current android release version from makefile
android_dessert()
{
    # e.g. DEFAULT_PLATFORM_VERSION := QPR1
    eval $(cat "${ANDROID_BUILD_TOP}"/build/make/core/version_defaults.mk \
        | grep 'DEFAULT_PLATFORM_VERSION :=' | sed -e 's/[ :]//g')
    echo ${DEFAULT_PLATFORM_VERSION}
}

# fetch live image list from GCP
live_image_list()
{
    check_required_utils "gsutil"
    gsutil cp gs://${BUCKET}/gsi.html .
}

# get file name according to current lunch
live_image_name()
{
    local dessert=$(android_dessert)
    local ts=$(date +%Y%m%d)
    local name=${dessert}.${TARGET_PRODUCT}-${TARGET_BUILD_VARIANT}.${ts}
    echo ${name}
}

# upload the live image and set ACLs to public
live_image_upload()
{
    check_required_utils "gsutil"
    local name=$(live_image_name)
    gsutil cp ${OUT}/${name}.raw.gz gs://${BUCKET}
    gsutil acl ch -u AllUsers:R gs://${BUCKET}/${name}.raw.gz
    gsutil cp gsi.html gs://${BUCKET}
    gsutil acl ch -u AllUsers:R gs://${BUCKET}/gsi.html
}

# generate the live image according to the specified file
live_image_gen_file()
{
    check_required_utils "gsutil simg2img gzip"
    local title="$1"
    local system_img="$2"
    local prefix="$3"
    simg2img ${system_img} ${prefix}.raw
    local size=$(du ${prefix}.raw | cut -f1)
    gzip -c ${prefix}.raw > ${prefix}.raw.gz
    echo $(basename ${prefix}.raw.gz) ${size} ${title}
}

# generate the live image according to current lunch
live_image_gen()
{
    local title=$(echo ${TARGET_PRODUCT} | sed -e 's/_/\//g' | tr a-z A-Z)
    local name=$(live_image_name)
    live_image_gen_file ${title} ${OUT}/system.img ${OUT}/${name}
}

# upload the system image according to current lunch setting
live_image_upload_current_lunch()
{
    local name=$(live_image_name)
    echo "TARGET_PRODUCT=${TARGET_PRODUCT}"
    echo "TARGET_BUILD_VARIANT=${TARGET_BUILD_VARIANT}"
    live_image_list
    echo "generate ${name}"
    local append=$(live_image_gen)
    sed -e "/${name}/d" -i gsi.html
    echo ${append} >> gsi.html
    live_image_upload
}
