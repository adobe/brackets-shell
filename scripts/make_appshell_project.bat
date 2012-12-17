@echo off


:: Build the project
echo Building Visual Studio projects
gyp/gyp appshell.gyp -I common.gypi --depth=.
