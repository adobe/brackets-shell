@ECHO OFF

REM - Check for OS. XP needs special treatment since its support for symlinks is limited
ver | findstr /i "5\.1\." > nul
IF %ERRORLEVEL% EQU 0 GOTO XPVersion
GOTO DefaultVersion

:DefaultVersion

ECHO Removing links to the Brackets repository

REM Remove existing links to dev folder
rmdir Release\dev
rmdir Debug\dev


ECHO Removing links to CEF

REM Remove existing links
rmdir Debug include lib libcef_dll Release


ECHO Creating links to CEF

REM Make new links
mklink /j Debug deps\cef\Debug
mklink /j include deps\cef\include
mklink /j lib deps\cef\lib
mklink /j libcef_dll deps\cef\libcef_dll
mklink /j Release deps\cef\Release

GOTO Exit

:XPVersion

junction >NUL 2>&1
IF %ERRORLEVEL% EQU 9009 GOTO XPDownloadJunctionTool
GOTO XPCreateLinks

:XPDownloadJunctionTool

ECHO Downloading Junction from sysinternals.com
curl -# "http://download.sysinternals.com/files/Junction.zip" > Junction.zip
ECHO Extracting ZIP file
unzip -qo Junction.zip junction.exe
rm Junction.zip

GOTO XPCreateLinks

:XPCreateLinks

REM On XP, rmdir may delete the entire Brackets repo if 'Release\dev' is a junction point.

ECHO Removing links to the Brackets repository

REM Remove existing links to dev folder
junction -d Release\dev >NUL
junction -d Debug\dev >NUL

REM If the above paths weren't links, remove them as folders
rmdir Release\dev >NUL 2>&1
rmdir Debug\dev >NUL 2>&1


ECHO Removing links to CEF

REM Remove existing links
junction -d Debug >NUL
junction -d include >NUL
junction -d lib >NUL
junction -d libcef_dll >NUL
junction -d Release >NUL

REM If the above paths weren't links, remove them as folders
rmdir Debug include lib libcef_dll Release >NUL 2>&1


ECHO Creating links to CEF

REM Make new links
junction Debug deps\cef\xDebug >NUL
junction include deps\cef\include >NUL
junction lib deps\cef\lib >NUL
junction libcef_dll deps\cef\libcef_dll >NUL
junction Release deps\cef\Release >NUL

GOTO Exit

:Exit
exit /b
