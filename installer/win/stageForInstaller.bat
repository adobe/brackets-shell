@echo off

:: Prerequisites:
:: 1. Pull down the brackets repo
:: 2. Set BRACKETS_WWW_SRC to the path to brackets\src inside that repo (with NO trailing slash)
:: 3. Build the Windows build of brackets-shell - see scripts\make_appshell_project.bat
::    (should create a brackets-shell\Release dir)


:: Check that BRACKETS_WWW_SRC var is defined and points to a dir that exists
:: (we can't only check the latter since an undefined var is "", which DOES 'exist')
IF NOT DEFINED BRACKETS_WWW_SRC GOTO NO_WWW_SRC
IF NOT EXIST %BRACKETS_WWW_SRC% GOTO NO_WWW_SRC


:: Folder structure before staging:
::   brackets-shell
::      installer
::         mac
::         win  <-- current directory
::      Release
::         Brackets.exe
::         locales
::         ...
::      ...
::   brackets
::      src   <-- BRACKETS_WWW_SRC
::      test

:: Folder structure after staging:
::   brackets-shell
::      installer
::         win
::            staging
::               Brackets.exe
::               locales
::               ...
::               www
::                  <contents of BRACKETS_WWW_SRC>
::      ...


:: Notes on xcopy usage...
::
:: xcopy uses a very primitive exclusion mechanism. The exclusions have to be stored in a file (can't be
:: passed on command line), and they are matched as literal strings (no wildcards). They can't use spaces
:: (even with quotes), and the path containing the exclude text file also cannot contain spaces. The exclude
:: strings are matched against the absolute path+name of each file AND directory being copied.
::
:: The source path is expected to have NO trailing slash (otherwise "Invalid path"); but the destination path
:: *must* have a trailing slash or else xcopy will prompt to disambiguate dest folder vs. path (unless you pass /i).
::
:: For detailed Windows scripting documentation, see:
:: http://www.microsoft.com/resources/documentation/windows/xp/all/proddocs/en-us/ntcmds.mspx?mfr=true


:: Remove any old bits from the staging dir
IF NOT EXIST staging\ GOTO STAGING_CLEARED

echo Clearing old staging folder %CD%\staging ...
rmdir staging\ /s /q

:STAGING_CLEARED


:: Copy Release of brackets-shell to staging
echo Copying brackets-shell build from %CD%\..\..\Release ...

(
    echo .pdb
    echo \obj\
    echo \lib\
    echo avformat-54.dll
    echo avutil-51.dll
    echo avcodec-54.dll
    echo d3dcompiler_43.dll
    echo d3dx9_43.dll
    echo libEGL.dll
    echo libGLESv2.dll
    echo cefclient.exe
    echo debug.log
) > shell_excludes.tmp
xcopy ..\..\Release staging /s /i /exclude:shell_excludes.tmp
del shell_excludes.tmp


:: Copy BRACKETS_WWW_SRC to staging\www
:: excluding .git*
echo Copying brackets source from %BRACKETS_WWW_SRC% ...

echo .git > www_excludes.tmp
xcopy %BRACKETS_WWW_SRC% staging\www /s /i /exclude:www_excludes.tmp
del www_excludes.tmp


echo.
echo Done staging Brackets for Windows! (Run installer generator script next)

GOTO END



:NO_WWW_SRC
echo Error: Unable to locate BRACKETS_WWW_SRC. Make sure environment variable is set, and is pointing to your brackets\src directory (with NO trailing slash).

:END