# Merge build with CSI script

### Description

Script and additional files for merging files of CSI and Partial build
without system image, then generate image archive for flash. The script is using
releasetools/merge_target_files, and it will be used for making mixed build
target at AB.

### Files
* merge_with_csi.sh
  * shell script for execute
* framework_files.list
  * list of target files that is in system and will be placed into merged target
* partial_files.list
  * list of target files that is in partial build and will be placed into
  merged target
* misc_info_keys.list
  * list of keys of system build's META/misc_info.txt that will be placed into
  merged build. Default value of merged META/misc_info.txt comes from partial
  build

### Usage
```
merge_with_csi.sh \
  --framework-dir cache/csi \
  --vendor-dir cache/partial \
  --output-dir out/dist \
  --create-img-archive --create-android-info
```