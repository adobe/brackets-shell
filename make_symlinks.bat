REM Remove existing links
rmdir Debug include lib libcef_dll Release tools

REM Make new links
mklink /d Debug deps\cef\Debug
mklink /d include deps\cef\include
mklink /d lib deps\cef\lib
mklink /d libcef_dll deps\cef\libcef_dll
mklink /d Release deps\cef\Release
mklink /d tools deps\cef\tools
