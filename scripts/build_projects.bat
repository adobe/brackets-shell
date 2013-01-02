:: Batch file that sets up Visual Studio command line tools and builds appshell.sln
:: This file should *not* be called directly. It is called by the build.sh script.

call "%VS100COMNTOOLS%/vsvars32.bat"
msbuild.exe appshell.sln /t:Clean /p:Platform=Win32 /p:Configuration=Release
msbuild.exe appshell.sln /t:Build /p:Platform=Win32 /p:Configuration=Release
exit /b
