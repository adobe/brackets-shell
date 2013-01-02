ECHO OFF

REM - Check for OS. This script works in Vista or later.
REM   Running it on XP may delete the entire Brackets repo if 'Release\dev' is a junction point.
ver | findstr /i "5\.1\." > nul
IF %ERRORLEVEL% EQU 0 GOTO XPNotSupported

REM Remove existing links to dev folder
rmdir Release\dev
rmdir Debug\dev

REM Remove existing links
rmdir Debug include lib libcef_dll Release

REM Make new links
mklink /j Debug deps\cef\Debug
mklink /j include deps\cef\include
mklink /j lib deps\cef\lib
mklink /j libcef_dll deps\cef\libcef_dll
mklink /j Release deps\cef\Release

GOTO Exit


:XPNotSupported
ECHO Sorry, this script doesn't run in Windows XP. Build brackets-shell on a newer computer and
ECHO manually place the results in brackets-shell\Release.


:Exit
exit /b
