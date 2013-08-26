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
VERSION=`. linux/INFO; echo ${MINOR_VERSION}`
# deb package name
PKG_NAME="brackets-sprint${VERSION}-0ubuntu1-${ARCH}.deb" 


#
#  FUNCTIONS
#

# create build filesystem
prepare() {
    rm -rf package-root
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

    # copy staging directory
    cp -dpr --no-preserve=ownership \
        "../staging" \
        "package-root/opt/brackets"

    # copy files from resource directory
    cp -dpr --no-preserve=ownership \
        "../linux/brackets" \
        "package-root/opt/brackets/brackets"
    ln -s "/opt/brackets/brackets" "package-root/usr/bin/brackets"
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
    chmod 755 package-root/opt/brackets/brackets
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
    # update control file
    cd "package-root/DEBIAN"
    awk -v arch=${ARCH} -v ver=${VERSION} -v size=${_SIZE} \
       'BEGIN{FS=":";OFS=":"}{ \
        sub(/^Version:.*/, $1": "'ver'); \
        sub(/^Architecture:.*/, $1": "'arch'); \
        sub(/^Installed-Size:.*/, $1": "'size'); \
        print}' control > control.temp

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
