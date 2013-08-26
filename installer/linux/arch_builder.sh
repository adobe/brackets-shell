#!/bin/bash
#
#  Package builder for Arch Linux
#

#
#  GLOBAL VARIABLES
#

# brackets version number
VERSION=`cat linux/version | tr -d " \t\n\r"`


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
}

# update pkgbuild file
pkgbuild() {
    awk 'BEGIN{FS="=";OFS="="} \
        /^pkgver=/{$2="sprint"'"${VERSION}"'}{print}' PKGBUILD > PKGBUILD.new
    rm -f PKGBUILD
    mv PKGBUILD.new PKGBUILD
}


#
# MAIN
#
cd "arch"
prepare
makepkg
clean
