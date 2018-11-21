#!/bin/bash
#
#   usage: boot-live-image.sh [system.img]
#
#       when used /wo the 1st argument, fetch the image list from gcp for user
#       to select.
#
gcp_bucket="https://storage.googleapis.com/treble-gsi"
image_list=gsi.html
cache_dir=${HOME}/Downloads
file=$1
if [ -z $file ]; then
    wget ${gcp_bucket}/${image_list} -O ${cache_dir}/${image_list} >/dev/null 2>&1
    n=0
    while read l ; do
        file=`echo $l | cut -d ' ' -f1`
        title=`echo $l | cut -d ' ' -f3,4,5,6`
        echo [$n] $title
        eval "s${n}=$file"
        n=$(($n+1))
    done < ${cache_dir}/${image_list}
    file=""
    while [ -z $file ]; do
        echo -n "select?"
        read select
        eval "file=\$s$select"
    done
    url=${gcp_bucket}/$file
    cache=${cache_dir}/${file}
    echo ${cache}
    if ! [ -f ${cache} ]; then
        echo "fetch..."
        wget $url -O ${cache_dir}/${file}
        cp ${cache} .
        echo "gzip..."
        gunzip ${file}
    fi
    raw=${file%.*}
else
    raw=${file%.*}.raw
    echo simg2img $file ${raw}
    simg2img $file ${raw}
fi
ls $raw
adb root
n=`adb shell cat /sys/class/zram-control/hot_add`
echo "using zram$n"
adb shell "echo 8G > /sys/block/zram$n/disksize"

echo "#"
echo "# Enter to continue following commands"
echo "#"
echo adb push ${raw} /dev/block/zram$n
read ans
adb push ${raw} /dev/block/zram$n
adb shell sync

#echo ""
#echo "# start checking point"
#adb shell "vdc --wait checkpoint startCheckpoint 1"

echo "#"
echo "# Enter to continue following commands"
echo "#"
echo adb reboot
read ans
adb reboot
