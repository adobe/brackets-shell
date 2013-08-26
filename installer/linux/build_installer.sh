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
fi;


if [[ $DISTRO == 'ubuntu' ]]; then
    bash ubuntu_installer.sh
fi;
