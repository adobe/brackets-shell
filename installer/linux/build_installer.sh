#!/bin/bash
#
#  Build installer for linux
# 
#  When add new distro check other possible release files:
#    http://linuxmafia.com/faq/Admin/release-files.html


DISTRO="unknown"


# get distribution name from /etc/*release file
if [[ -f /etc/os-release ]]; then
    . /etc/os-release; DISTRO=$ID
elif [[ -f /etc/lsb_release ]]; then
    . /etc/lsb_release; DISTRO=$DISTRIB_ID
else
    echo "Error: non-standard /etc/*release files are currently not supported" 2>&1
    exit 1
fi;


# run distro-specified builder
if [[ -f ${DISTRO}_builder.sh ]]; then
    bash ${DISTRO}_builder.sh
else
    echo "Error: ${DISTRO}_builder.sh not found" 2>&1
    echo "       Can't build package for distribution \"${DISTRO}\"" 2>&1
    exit 1
fi;
