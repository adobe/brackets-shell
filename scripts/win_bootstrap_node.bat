set PATH=%CD%\deps\node;%PATH%
cd appshell\node-core
cmd.exe /C npm.cmd install
cd ..\..
exit /b
