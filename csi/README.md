# Script for merging partial build with CSI

### Description

Script and additional files for merging files of CSI and Partial build
without system image, plus generate image archive for flash. The script is using
releasetools/merge_target_files, and it will be used for making mixed build
target at AB.

### Files
* merge_with_csi.sh
  * shell script to execute merging
* framework_files.list
  * list of target files of system that needs to be at merged target
* partial_files.list
  * list of target files of partial build that needs to be at merged target
* misc_info_keys.list
  * list of keys of system build's META/misc_info.txt that needs to be placed at
  merged build. Default value of merged META/misc_info.txt comes from partial
  build's META/misc_info.txt

### Usage
```
merge_with_csi.sh \
  --framework-dir cache/csi \
  --vendor-dir cache/partial \
  --output-dir out/dist \
  --create-img-archive --create-android-info
```