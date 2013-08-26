#!/bin/bash
#
#  Package builder for Ubuntu
#


#
#  GLOBAL VARIABLES
#

# current architecture
if [[ `uname -m` == "x86_64" ]]; then
    ARCH="amd64"
else
    ARCH="i386"
fi;
# brackets version number
VERSION=`cat linux/version | tr -d " \t\n\r"`
# deb package name
PKG_NAME="brackets-sprint${VERSION}-0ubuntu1-${ARCH}.deb" 


#
#  FUNCTIONS
#

# create build filesystem
prepare() {
    # create dirs
    mkdir -p package-root/DEBIAN
    mkdir -p package-root/opt
    mkdir -p package-root/usr/bin
    mkdir -p package-root/usr/share/menu
    mkdir -p package-root/usr/share/icons/hicolor/scalable/apps
    mkdir -p package-root/usr/share/doc/brackets

    # copy package files
    cp -dpr --no-preserve=ownership \
        "control" \
        "package-root/DEBIAN/control"
    cp -dpr --no-preserve=ownership \
        "postinst" \
        "package-root/DEBIAN/postinst"
    cp -dpr --no-preserve=ownership \
        "prerm" \
        "package-root/DEBIAN/prerm"
    cp -dpr --no-preserve=ownership \
        "postrm" \
        "package-root/DEBIAN/postrm"

    # copy src directory
    cp -dpr --no-preserve=ownership \
        "../src" \
        "package-root/opt/brackets"

    # copy files from resource directory
    cp -dpr --no-preserve=ownership \
        "../linux/brackets" \
        "package-root/usr/bin/brackets"
    cp -dpr --no-preserve=ownership \
        "../linux/brackets.desktop" \
        "package-root/opt/brackets/brackets.desktop"
    cp -dpr --no-preserve=ownership \
        "../linux/brackets.menu" \
        "package-root/usr/share/menu/brackets.menu"
    cp -dpr --no-preserve=ownership \
        "../linux/brackets.svg" \
        "package-root/usr/share/icons/hicolor/scalable/apps/brackets.svg"
    cp -dpr --no-preserve=ownership \
        "../linux/license" \
        "package-root/usr/share/doc/brackets/copyright"

    # grunt-contrib-copy doesn't preserve permissions
    # https://github.com/gruntjs/grunt/issues/615 
    chmod 755 package-root/usr/bin/brackets
    chmod 755 package-root/opt/brackets/Brackets
    chmod 755 package-root/opt/brackets/Brackets-node
    chmod 755 package-root/DEBIAN/prerm
    chmod 755 package-root/DEBIAN/postrm
    chmod 755 package-root/DEBIAN/postinst

    # set permissions on subdirectories
    find . -type d -exec chmod 755 {} \;

    # update control file
    control

    # delete old package
    rm -f ../$PKG_NAME
}

# postbuild clean
clean() {
    mv package-root.deb ../$PKG_NAME
    rm -rf package-root
}

# update control file
control() {
    # size of all files in opt dir (in kB)
    local _SIZE=`du -shb package-root/opt/ | \
                awk '{printf "%.0f\n", $1 / 1000}'`
    # update control file (very ugly way)
    cd "package-root/DEBIAN"
    awk 'BEGIN{FS=":";OFS=":"} \
        /^Version:/{$2=" "'"${VERSION}"'}{print}' control > control.temp
    rm -f control
    mv control.temp control
    awk 'BEGIN{FS=":";OFS=":"} \
        /^Architecture:/{$2=" "'"${ARCH}"'}{print}' control > control.temp
    rm -f control
    mv control.temp control
    awk 'BEGIN{FS=":";OFS=":"} \
        /^Installed-Size:/{$2=" "'"${_SIZE}"'}{print}' control > control.temp
    rm -f control
    mv control.temp control
    cd ../..
}


#
# MAIN
#

cd "ubuntu"
prepare
fakeroot dpkg-deb --build package-root
clean
