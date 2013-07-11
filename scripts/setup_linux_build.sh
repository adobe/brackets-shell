#!/bin/bash

MACHINE_TYPE=`uname -m`

# SYSTEM DEPENDENCIES
# install development dependencies
sudo apt-get install --assume-yes wget p7zip-full libnss3-1d libnspr4-0d gyp gtk+-2.0

# BUILD DEPENDENCIES

mkdir deps
mkdir downloads
pushd downloads


# download Adobe CEF build
ZIPNAME="cef_binary_3.1453.1255_linux32"

if [ ! -f cef.zip ]; then
  wget -O cef.7z "https://drive.google.com/uc?export=download&id=0B7as0diokeHxMVpTOTM5NUJwemM"
fi

# extract CEF 
rm -rf ../deps/cef
7za x cef.7z
mv $ZIPNAME ../deps/cef

# if 64bit platform, pull down 64bit CEF client and use 64bit CEF library.
# As www.magpcss.net/cef-downalds requires CAPTCHA, this will need to be done manually
# 
#if [ ${MACHINE_TYPE} == 'x86_64' ]; then
#  wget -O cef_binary_3.1453.1255_linux64_client.7z "http://www.magpcss.net/cef-downloads/index.php?file=cef_binary_3.1453.1255_linux64_client.7z"
#
#fi

# download node binary

if [ ${MACHINE_TYPE} == 'x86_64' ]; then
  NODE_FILE='node-v0.8.20-linux-x64'
else
  NODE_FILE='node-v0.8.20-linux-x86'
fi
wget "http://nodejs.org/dist/v0.8.20/$NODE_FILE.tar.gz"
tar xvzf $NODE_FILE.tar.gz
mv $NODE_FILE ../deps/node

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


# NB - After this script has completed successfully, 
# 
# - if 64bit, replace deps/cef/libcef.so with the 64bit libcef.so in the linux 64bit client.7z
# - in the brackets-shell directory, run 'make'
# - there should be a binary named 'Brackets' in out/Release. In the same directory, add
#   symlinks to allow the appshell to find the cloned brackets directory
#      
#      ln -s /path/to/brackets dev
#      ln -s /path/to/brackets/samples .
