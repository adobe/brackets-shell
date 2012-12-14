REM Remove existing links
rmdir Debug include lib libcef_dll Release

REM Make new links
mklink /j Debug deps\cef\Debug
mklink /j include deps\cef\include
mklink /j lib deps\cef\lib
mklink /j libcef_dll deps\cef\libcef_dll
mklink /j Release deps\cef\Release

exit
