#!/bin/bash -ex

if [ -z "${ANDROID_BUILD_TOP}" ]; then
  echo 1>&2 "error: ANDROID_BUILD_TOP must be set"
  exit 1
fi

header-abi-dumper example.cpp -I. -o example.sdump --

shared_object="prebuilts/libversion_script_example.so"
version_script="example.map.txt"

header-abi-linker example.sdump \
  -o example.lsdump \
  -so "${shared_object}" \
  -v "${version_script}"

sed -i "s#${ANDROID_BUILD_TOP}/##g" example.lsdump

header-abi-linker example.sdump \
  -o example-no-private.lsdump \
  -so "${shared_object}" \
  -v "${version_script}" \
  --exclude-symbol-version "LIBVERSION_SCRIPT_EXAMPLE_PRIVATE"

sed -i "s#${ANDROID_BUILD_TOP}/##g" example-no-private.lsdump

header-abi-linker example.sdump \
  -o example-no-mytag.lsdump \
  -so "${shared_object}" \
  -v "${version_script}" \
  --exclude-symbol-tag "mytag"

sed -i "s#${ANDROID_BUILD_TOP}/##g" example-no-mytag.lsdump

rm example.sdump
