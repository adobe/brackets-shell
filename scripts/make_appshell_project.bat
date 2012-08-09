@echo off

:: Check for CHROMIUM_SRC_PATH
IF NOT EXIST %CHROMIUM_SRC_PATH% THEN GOTO NOCHROMIUMPATH

:: Build the project
echo Building Visual Studio projects
%CHROMIUM_SRC_PATH%/tools/gyp/gyp appshell.gyp -I %CHROMIUM_SRC_PATH%/build/common.gypi --depth=%CHROMIUM_SRC_PATH%/

:NOCHROMIUMPATH
echo CHROMIUM_SRC_PATH environment variable is not set. Please make sure it is pointing to your chromium 'src' directory and does not have a trailing '/'
:ENDs