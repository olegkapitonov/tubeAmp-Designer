#!/bin/bash

cd ..
mkdir Appimage/AppDir
meson --prefix=/usr Appimage/builddir
DESTDIR=`realpath ./Appimage/AppDir` ninja -C Appimage/builddir install

cd Appimage

wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy-plugin-qt-x86_64.AppImage

./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage

rm linuxdeploy-plugin-qt-x86_64.AppImage
rm linuxdeploy-x86_64.AppImage
rm -rf AppDir
rm -rf builddir
