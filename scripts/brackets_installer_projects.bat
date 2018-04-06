:: Batch file that creates the custom DLLs required to build the windows installers
:: This file should *not* be called directly. It is called by the build.sh script.
@ECHO OFF

IF DEFINED VS140COMNTOOLS GOTO VS2015
IF DEFINED VS120COMNTOOLS GOTO VS2013
IF DEFINED VS110COMNTOOLS GOTO VS2012
IF DEFINED VS100COMNTOOLS GOTO VS2010

:VSNotInstalled
ECHO Visual Studio 2010, 2012, 2013 or 2015 must be installed.
GOTO Error

:VS2015
call "%VS140COMNTOOLS%/vsvars32.bat"
GOTO Build

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
msbuild.exe installer\win\LaunchBrackets\LaunchBracketsSolution.sln /nr:false /t:Clean /p:Platform=x86 /p:Configuration=Release
msbuild.exe installer\win\LaunchBrackets\LaunchBracketsSolution.sln /nr:false /t:Build /p:Platform=x86 /p:Configuration=Release

msbuild.exe installer\win\BracketsConfigurator\BracketsConfiguratorSolution.sln /nr:false /t:Clean /p:Platform=x86 /p:Configuration=Release
msbuild.exe installer\win\BracketsConfigurator\BracketsConfiguratorSolution.sln /nr:false /t:Build /p:Platform=x86 /p:Configuration=Release
exit /b %ERRORLEVEL%

:Error
exit /b 1
