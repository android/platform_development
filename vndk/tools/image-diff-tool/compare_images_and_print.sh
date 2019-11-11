#!/bin/bash

build/soong/soong_ui.bash --make-mode --skip-make compare_images
. build/envsetup.sh
lunch aosp_arm64
out/host/linux-x86/bin/compare_images $1 -u
echo
echo " - Common parts"
cat common.csv
echo
echo " - Different parts (that are whitelisted)"
cat whitelisted_diff.csv
echo
echo " - Different parts (that are not whitelisted)"
cat diff.csv

if [ -v DIST_DIR ]; then
  cp common.csv diff.csv whitelisted_diff.csv $DIST_DIR
fi

if [ "$(wc -l diff.csv | cut -d' ' -f1)" -gt "1" ]; then
  echo "" >&2
  echo "[ERROR] Unexpected diffing files" >&2
  echo "There are diffing files that are not ignored by whitelist." >&2
  echo "The whitelist may need to be updated." >&2
  exit 1
fi
