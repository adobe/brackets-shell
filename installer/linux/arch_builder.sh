#!/bin/bash
#
#  Package builder for Arch Linux
#

#
#  GLOBAL VARIABLES
#

# brackets version number
VERSION=`. linux/INFO; echo ${MINOR_VERSION}`


#
#  FUNCTIONS
#

# create build filesystem
prepare() {
    cp -dpr --no-preserve=ownership \
        "../staging/" \
        "src"
    cp -dpr --no-preserve=ownership \
        "../linux/." \
        "src"

    # update pkgbuild file
    pkgbuild

    # delete old package
    rm -f *pkg.tar*
}

# postbuild clean
clean() {
    mv *pkg.tar* ../
    rm -rf src
    rm -rf pkg
    rm -f PKGBUILD
}

# update pkgbuild file
pkgbuild() {
    awk -v ver=${VERSION} \
        'BEGIN{FS="=";OFS="="} {\
        sub(/^pkgver=.*/, $1"=sprint"'ver'); \
        print}' PKGBUILD.template > PKGBUILD
}


#
# MAIN
#
cd "arch"
prepare
makepkg
clean
