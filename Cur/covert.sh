#!/bin/bash
wavPath=$1
mp3Path=$2
srcPath=$3
destPath=$4
destMp3Path=$5
TransMode=$6
DelWavFlag=$7
BtrasWavbak=$8
bak='bak'
output=`lame --preset fast standard $wavPath $mp3Path`
if [ $BtrasWavbak=="1" ];then
   mv $wavPath $wavPath$bak
   echo "BtransWavbak=$BtransWavbak"
fi

if [ $TransMode=="1" ];then
        output=`sshpass -p jiang1996 ssh root@192.168.1.136 "[ -d $destMp3Path ]&& echo "ok" || mkdir -p $destMp3Path"`
        output=`sshpass -p jiang1996 scp -r $srcPath @$destPath`
        echo "TransMode=$TransMode"
fi

if [ $DelWavFlag=="1" ];then
    echo "DelWavFlag=$DelWavFlag"
fi
