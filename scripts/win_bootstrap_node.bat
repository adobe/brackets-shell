set PATH=%CD%\deps\node;%PATH%
cd appshell\server
cmd.exe /C npm.cmd install
cd ..\..
exit /b
