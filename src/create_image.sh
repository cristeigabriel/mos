#!/bin/bash
set -Eeuo pipefail

dd if=/dev/zero of=hdd.img count=20480 bs=512
VOLUME_NAME=hdd
DISK_NAME="$(hdiutil attach -nomount hdd.img)"
$(brew --prefix e2fsprogs)/sbin/mkfs.ext2 $DISK_NAME
hdiutil detach $DISK_NAME
hdiutil attach hdd.img -mountpoint /Volumes/$VOLUME_NAME
mkdir "/Volumes/${VOLUME_NAME}/dev"

mkdir "/Volumes/${VOLUME_NAME}/usr"
mkdir "/Volumes/${VOLUME_NAME}/usr/share"
cp -R assets/fonts "/Volumes/${VOLUME_NAME}/usr/share"
cp -R assets/images "/Volumes/${VOLUME_NAME}/usr/share"

cd apps/window_server && make clean && make
cd ../..
cd apps/terminal && make clean && make
cd ../..
cd apps/shell && make clean && make
cd ../..
cd apps/uname && make clean && make
cd ../..
cd apps/host && make clean && make
cd ../..
cd apps/calculator && make clean && make
cd ../..

mkdir "/Volumes/${VOLUME_NAME}/bin"
cp apps/window_server/window_server "/Volumes/${VOLUME_NAME}/bin"
cp apps/terminal/terminal "/Volumes/${VOLUME_NAME}/bin"
cp apps/shell/shell "/Volumes/${VOLUME_NAME}/bin"
cp apps/uname/uname "/Volumes/${VOLUME_NAME}/bin"
cp apps/host/host "/Volumes/${VOLUME_NAME}/bin"
cp apps/calculator/calculator "/Volumes/${VOLUME_NAME}/bin"

mkdir "/Volumes/${VOLUME_NAME}/etc"
cp apps/window_server/desktop.ini "/Volumes/${VOLUME_NAME}/etc"

mkdir "/Volumes/${VOLUME_NAME}/tmp"
cp assets/book.txt "/Volumes/${VOLUME_NAME}/tmp"

hdiutil detach $DISK_NAME

DISK_NAME="$(hdiutil attach -nomount hdd.img)"
$(brew --prefix e2fsprogs)/sbin/dumpe2fs $DISK_NAME
hdiutil detach $DISK_NAME
