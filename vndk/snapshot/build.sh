#!/bin/bash

echo "-----Source build/envsetup.h"
source build/envsetup.sh

echo "-----Generating VNDK snapshot for arm, arm64"
make -j vndk dist TARGET_PRODUCT=aosp_arm64_ab BOARD_VNDK_VERSION=current

echo "-----Generating VNDK snapshot for x86, x86_64"
make -j vndk dist TARGET_PRODUCT=aosp_x86_64 BOARD_VNDK_VERSION=current
