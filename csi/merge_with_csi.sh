#!/bin/bash -ex
usage() {
  echo
  echo "Merge CSI and partial build"
  echo
  echo "Create build artifact archive and image archive"
  echo "  by merging CSI and partial build of Pixel that was"
  echo "  built by using WITHOUT_SYSTEM_IMAGE=true"
  echo
  echo "Usage: merge_with_csi.sh [args]"
  echo
  echo "  --h, --help"
  echo "    Print help (this screen)"
  echo
  echo "  --framework-dir dir_name"
  echo "    (required) Path of directory that contains framework's target files archive"
  echo
  echo "  --vendor-dir dir_name"
  echo "    (required) Path of directory that contains partial build's target files archive"
  echo
  echo "  --output-dir dir_name"
  echo "    (required) Path of directory that result files stored"
  echo
  echo "  --create-img-archive"
  echo "    (optional) If specified, archive of image files will be generated"
  echo
  echo "  --create-android-info"
  echo "    (optional) If specified, put android-info.txt top of output-dir"
  echo
  echo "Example:"
  echo "  merge_with_csi.sh \\"
  echo "    --framework-dir cache/csi \\"
  echo "    --vendor-dir cache/partial \\"
  echo "    --output-dir out/dist \\"
  echo "    --create-img-archive --create-android-info"
  echo
}

print_error() {
  echo "ERROR: $1" >&2
}

exit_error() {
  print_error "$1"
  exit 1
}

exit_error_with_usage() {
  print_error "$1"
  usage
  exit 1
}

if [ ! -e "build/make/core/Makefile" ]; then
  echo "$0 must be run from the top of the tree"
  exit 1
fi

if [ -z "${ANDROID_BUILD_TOP}" ] || [ -z "${ANDROID_HOST_OUT}" ]; then
  echo "Environment variable is not set, run lunch"
  exit 1
fi

if [ ! -f $ANDROID_HOST_OUT/bin/merge_target_files ]; then
  $ANDROID_BUILD_TOP/build/soong/soong_ui.bash --make-mode merge_target_files
fi

function parse_options() {
    while [[ $# > 0 ]]
    do
    key="$1"
    case $key in
        -h|--help)
            usage
            exit 1
            ;;
        --framework-dir)
            readonly FRAMEWORK_DIR="$2"
            shift
            ;;
        --vendor-dir)
            readonly VENDOR_DIR="$2"
            shift
            ;;
        --output-dir)
            readonly OUTPUT_DIR="$2"
            shift
            ;;
        --create-img-archive)
            readonly CREATE_IMG_ARCHIVE=true
            ;;
        --create-android-info)
            readonly CREATE_ANDROID_INFO=true
            ;;
        *)
            exit_error_with_usage "Unknown option $key"
            ;;
        esac
        shift
    done
}

if [ $# -eq 0 ]; then
  usage
  exit 0
fi

parse_options $@

if [ -z "${FRAMEWORK_DIR}" ]; then
  print_error "--framework-dir option must be specified"
  has_invalid_option=true
fi

if [ -z "${VENDOR_DIR}" ]; then
  print_error "--vendor-dir option must be specified"
  has_invalid_option=true
fi

if [ -z "${OUTPUT_DIR}" ]; then
  print_error "--output-dir option must be specified"
  has_invalid_option=true
fi

if [ ! -z "${has_invalid_option}" ]; then
  exit_error_with_usage "There are invalid options"
fi

FRAMEWORK_TARGET_FILES="$(find "$FRAMEWORK_DIR" -name "*-target_files-*.zip" -print)"

if [ -z "${FRAMEWORK_TARGET_FILES}" ]; then
  exit_error "Cannot find framework target archive"
fi

VENDOR_TARGET_FILES="$(find "$VENDOR_DIR" -name "*-target_files-*.zip" -print)"

if [ -z "${VENDOR_TARGET_FILES}" ]; then
  exit_error "Cannot find vendor target archive"
fi

if [ ! -d "${OUTPUT_DIR}" ]; then
  mkdir -p $OUTPUT_DIR
fi

OUTPUT_TARGET_FILES_PATH=${OUTPUT_DIR}/$(basename $VENDOR_TARGET_FILES)

MERGE_TARGET_FILES_OPTIONS="--framework-target-files=${FRAMEWORK_TARGET_FILES}"
MERGE_TARGET_FILES_OPTIONS+=" --vendor-target-files=${VENDOR_TARGET_FILES}"
MERGE_TARGET_FILES_OPTIONS+=" --framework-item-list=${ANDROID_BUILD_TOP}/development/csi/framework_files.list"
MERGE_TARGET_FILES_OPTIONS+=" --framework-misc-info-keys=${ANDROID_BUILD_TOP}/development/csi/misc_info_keys.list"
MERGE_TARGET_FILES_OPTIONS+=" --vendor-item-list=${ANDROID_BUILD_TOP}/development/csi/partial_files.list"
MERGE_TARGET_FILES_OPTIONS+=" -s=${ANDROID_HOST_OUT}/bin"
MERGE_TARGET_FILES_OPTIONS+=" --path=${ANDROID_HOST_OUT}"
MERGE_TARGET_FILES_OPTIONS+=" --output-target-files=${OUTPUT_TARGET_FILES_PATH}"

if [ ! -z "${CREATE_IMG_ARCHIVE}" ]; then
  OUTPUT_IMG_FILES_PATH="$(echo "$OUTPUT_TARGET_FILES_PATH" | sed -ne "s/-target_files-/-img-/p")"
  MERGE_TARGET_FILES_OPTIONS+=" --output-img=${OUTPUT_IMG_FILES_PATH}"
fi

$ANDROID_HOST_OUT/bin/merge_target_files $MERGE_TARGET_FILES_OPTIONS

if [ ! -z "${CREATE_ANDROID_INFO}" ]; then
  unzip -p $OUTPUT_TARGET_FILES_PATH OTA/android-info.txt > $OUTPUT_DIR/android-info.txt
fi