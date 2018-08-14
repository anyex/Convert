#!/bin/bash
wavPath=$1
mp3Path=$2
srcPath=$3
destPath=$4
destMp3Path=$5
bak='bak'
output=`lame --preset fast standard $wavPath $mp3Path`
mv $wavPath $wavPath$bak
output=`sshpass -p jiang1996 ssh root@192.168.254.132 "[ -d $destMp3Path ]&& echo "ok" || mkdir -p $destMp3Path"`
output=`sshpass -p jiang1996 scp -r $srcPath @$destPath`
