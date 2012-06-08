#include "client_app.h"
#include "resource.h"

#include <string>

std::string ClientApp::GetExtensionJSSource()
{
    extern HINSTANCE hInst;

    HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(IDS_BRACKETS_EXTENSIONS), MAKEINTRESOURCE(256));
    DWORD dwSize;
    LPBYTE pBytes = NULL;

    if(hRes)
    {
        HGLOBAL hGlob = LoadResource(hInst, hRes);
        if(hGlob)
        {
            dwSize = SizeofResource(hInst, hRes);
            pBytes = (LPBYTE)LockResource(hGlob);
        }
    }

    if (pBytes) {
        std::string result((const char *)pBytes, dwSize);
        return result;
    }

    return "";
}
