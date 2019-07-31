#!/bin/bash
build/soong/soong_ui.bash --make-mode compare_images
compare_images -t $1 $2 -s SYSTEM
echo " - Common parts"
cat common.csv
echo
echo " - Different parts"
cat diff.csv
