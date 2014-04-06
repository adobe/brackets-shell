:: Batch file that sets up Visual Studio command line tools and builds appshell.sln
:: This file should *not* be called directly. It is called by the build.sh script.
ECHO OFF

IF DEFINED VS120COMNTOOLS GOTO VS2013
IF DEFINED VS110COMNTOOLS GOTO VS2012
IF DEFINED VS100COMNTOOLS GOTO VS2010

:VSNotInstalled
ECHO Visual Studio 2010, 2012 or 2013 must be installed.
GOTO Exit

:VS2013
call "%VS120COMNTOOLS%/vsvars32.bat"
GOTO Build

:VS2012
call "%VS110COMNTOOLS%/vsvars32.bat"
GOTO Build

:VS2010
call "%VS100COMNTOOLS%/vsvars32.bat"
GOTO Build

:Build
msbuild.exe appshell.sln /t:Clean /p:Platform=Win32 /p:Configuration=Release
msbuild.exe appshell.sln /t:Build /p:Platform=Win32 /p:Configuration=Release

:Exit
exit /b %ERRORLEVEL%
