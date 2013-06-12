#!/bin/bash

# SYSTEM DEPENDENCIES
# install development dependencies
sudo apt-get install --assume-yes p7zip-full libnss3-1d libnspr4-0d gyp gtk+-2.0

# BUILD DEPENDENCIES
# download CEF
zipname="cef_binary_3.1453.1255_linux32"

mkdir deps
pushd deps

if [ ! -f cef.zip ]; then
  wget -O cef.7z "https://drive.google.com/uc?export=download&id=0B7as0diokeHxMVpTOTM5NUJwemM"
fi

# extract CEF 
rm -rf cef
7za x cef.7z
mv $zipname cef

popd

# make symlinks to CEF
# Remove existing links
rm -f Debug include libcef_dll Release Resources tools

# Make new links
# ln -s deps/cef/Debug/ Debug
ln -s deps/cef/include/ include
ln -s deps/cef/libcef_dll/ libcef_dll
ln -s deps/cef/Release/ Release
ln -s deps/cef/Resources/ Resources
# ln -s deps/cef/tools/ tools

# CREATE MAKEFILE AND BUILD
# copy appshell.gyp
cp appshell.gyp.txt appshell.gyp

gyp --depth .