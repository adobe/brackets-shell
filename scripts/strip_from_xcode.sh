# Please remove this script and back out the changes that have been
# introduced by the commit, that originally added this file to the
# repository.
# The reason why we have this file and needed to undef
# SYSTEM_DEVELOPER_BIN_DIR is the following:
# The script in tools/strip_save_dsym determines the path to the strip utility
# by examining the environment variable SYSTEM_DEVELOPER_BIN_DIR (which is defined by XCode).
# XCode 5 changed the directory where all the devtools are located.
# The path to strip is constructed by appending 'strip' to SYSTEM_DEVELOPER_BIN_DIR.
# Since this location doesn't have a copy strip anymore, the script fails with an error and the whole
# build of the shell fails too.
# Setting SYSTEM_DEVELOPER_BIN_DIR= makes the script use a fallback solution and use /usr/bin/strip.
#
# XCode 5 defines an environment variable DT_TOOLCHAIN_DIR that points to the root directory
# where all the devtools are now installed. This environment variable should be used by the Chromium team
# to determine the devtools location.
# We need to go with this solution to stay compatible to XCode 4.
#
# This script has been configured as a postbuild step in common.gypi. This needs to be changed back too.

SYSTEM_DEVELOPER_BIN_DIR=
# the working directory is the project root
exec tools/strip_from_xcode
exit 1
