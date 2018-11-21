#!/bin/bash

# name of the GCP project
PROJECT_ID=${PROJECT_ID:-treble-keystone}

# name of the GCP storage bucket
BUCKET=${BUCKET:-treble-gsi}

function gcp_login
{
    gcloud config set project ${PROJECT_ID}
    gcloud auth login
}

# fetch live image list from GCP
function ali_list
{
    gsutil cp gs://${BUCKET}/gsi.html .
}

# fetch live image list from GCP
function ali_name
{
    local dessert=$1
    local ts=$(date +%Y%m%d)
    local name=${dessert}.${TARGET_PRODUCT}-${TARGET_BUILD_VARIANT}.${ts}
    echo $name
}

# generate the live image according to current lunch
function ali_gen
{
    local dessert=$1
    local title=$(echo ${TARGET_PRODUCT} | sed -e 's/_/\//g' | tr a-z A-Z)
    local name=$(ali_name ${dessert})
    simg2img ${OUT}/system.img ${OUT}/${name}.raw
    local size=$(du ${OUT}/${name}.raw | cut -f1)
    rm -f ${OUT}/${name}.raw.gz
    gzip ${OUT}/${name}.raw
    echo ${name}.raw.gz ${size} ${title}
}

# upload the live image and set ACLs to public
function ali_upload
{
    local dessert=$1
    local name=$(ali_name ${dessert})

    gsutil cp ${OUT}/${name}.raw.gz gs://${BUCKET}
    gsutil acl ch -u AllUsers:R gs://${BUCKET}/${name}.raw.gz

    gsutil cp gsi.html gs://${BUCKET}
    gsutil acl ch -u AllUsers:R  gs://${BUCKET}/gsi.html
}

function ali_all
{
    local dessert=$1
    local name=$(ali_name ${dessert})
    echo "dessert=${dessert}"
    echo "TARGET_PRODUCT=${TARGET_PRODUCT}"
    echo "TARGET_BUILD_VARIANT=${TARGET_BUILD_VARIANT}"
    ali_list
    echo "generate ${name}"
    local append=$(ali_gen ${dessert})
    sed -e "/${name}/d" -i gsi.html
    echo $append >> gsi.html
    ali_upload
}
