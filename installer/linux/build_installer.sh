#!/bin/bash
#
#  Build installer for linux
# 
#  When add new distro check other possible release files:
#    http://linuxmafia.com/faq/Admin/release-files.html


if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    DISTRO=$ID
elif [[ -f /etc/lsb_release ]]; then
    . /etc/lsb_release
    DISTRO=$DISTRIB_ID
else
    echo "Error: non-standard /etc/*release file are currently not supported" 2>&1
    exit 1
fi;


if [[ $DISTRO == "ubuntu" ]]; then
    bash ubuntu_builder.sh
elif [[ $DISTRO == "arch" ]]; then
    bash arch_builder.sh
else
    echo "Error: can't build package for distribution \"${DISTRO}\"" 2>&1
    exit 1
fi;
