/*
 * Copyright (c) 2016 - present Adobe Systems Incorporated. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "appshell/appshell_helpers.h"

#include <algorithm>
#include <commdlg.h>
#include <ShlObj.h>
#include <sstream>
#include <windows.h>

#include "include/cef_version.h"
#include "appshell/common/client_switches.h"
#include "appshell/browser/resource.h"
#include "config.h"

namespace appshell {

static wchar_t szInitialUrl[MAX_UNC_PATH] = {0};
DWORD g_appStartupTime = timeGetTime();

double GetElapsedMilliseconds()
{
    return (timeGetTime() - g_appStartupTime);
}

CefString GetCurrentLanguage()
{
    // Get the user's selected language
    // Defaults to the system installed language if not using MUI.
    LANGID langID = GetUserDefaultUILanguage();

    // Convert LANGID to a RFC 4646 language tag (per navigator.language)
    int langSize = GetLocaleInfo(langID, LOCALE_SISO639LANGNAME, NULL, 0);
    int countrySize = GetLocaleInfo(langID, LOCALE_SISO3166CTRYNAME, NULL, 0);

    wchar_t *lang = new wchar_t[langSize + countrySize + 1];
    wchar_t *country = new wchar_t[countrySize];

    GetLocaleInfo(langID, LOCALE_SISO639LANGNAME, lang, langSize);
    GetLocaleInfo(langID, LOCALE_SISO3166CTRYNAME, country, countrySize);

    // add hyphen
    wcscat(wcscat(lang, L"-"), country);
    std::wstring locale(lang);

    delete [] lang;
    delete [] country;

    return CefString(locale);
}

std::string GetExtensionJSSource()
{
    HINSTANCE hInst = GetModuleHandle(NULL);

    HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(IDS_APPSHELL_EXTENSIONS), MAKEINTRESOURCE(256));
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

void PopulateSettings(CefSettings* settings) {
    // Do nothing.
}

// Helper function for AppGetProductVersionString. Reads version info from
// VERSIONINFO and writes it into the passed in std::wstring.
void GetFileVersionString(std::wstring &retVersion) {
    DWORD dwSize = 0;
    BYTE *pVersionInfo = NULL;
    VS_FIXEDFILEINFO *pFileInfo = NULL;
    UINT pLenFileInfo = 0;

    HMODULE module = GetModuleHandle(NULL);
    TCHAR executablePath[MAX_UNC_PATH];
    GetModuleFileName(module, executablePath, MAX_UNC_PATH);

    dwSize = GetFileVersionInfoSize(executablePath, NULL);
    if (dwSize == 0) {
        return;
    }

    pVersionInfo = new BYTE[dwSize];

    if (!GetFileVersionInfo(executablePath, 0, dwSize, pVersionInfo)) {
        delete[] pVersionInfo;
        return;
    }

    if (!VerQueryValue(pVersionInfo, TEXT("\\"), (LPVOID*)&pFileInfo, &pLenFileInfo)) {
        delete[] pVersionInfo;
        return;
    }

    int major = (pFileInfo->dwFileVersionMS >> 16) & 0xffff;
    int minor = (pFileInfo->dwFileVersionMS) & 0xffff;
    int hotfix = (pFileInfo->dwFileVersionLS >> 16) & 0xffff;
    int other = (pFileInfo->dwFileVersionLS) & 0xffff;

    delete[] pVersionInfo;

    std::wostringstream versionStream(L"");
    versionStream << major << L"." << minor << L"." << hotfix << L"." << other;
    retVersion = versionStream.str();
}

CefString AppGetProductVersionString() {
    std::wstring s(APP_NAME);
    size_t i = s.find(L" ");
    while (i != std::wstring::npos) {
        s.erase(i, 1);
        i = s.find(L" ");
    }
    std::wstring version(L"");
    GetFileVersionString(version);
    s.append(L"/");
    s.append(version);
    return CefString(s);
}

CefString AppGetChromiumVersionString() {
    std::wostringstream versionStream(L"");
    versionStream << L"Chrome/" << cef_version_info(2) << L"." << cef_version_info(3)
                  << L"." << cef_version_info(4) << L"." << cef_version_info(5);

    return CefString(versionStream.str());
}

CefString AppGetSupportDirectory()
{
    wchar_t dataPath[MAX_UNC_PATH];
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, dataPath);

    std::wstring appSupportPath = dataPath;
    appSupportPath +=  L"\\" GROUP_NAME APP_NAME;

    // Convert '\\' to '/'
    replace(appSupportPath.begin(), appSupportPath.end(), '\\', '/');

    return CefString(appSupportPath);
}

CefString AppGetDocumentsDirectory()
{
    wchar_t dataPath[MAX_UNC_PATH] = {0};
    SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, dataPath);
    std::wstring appUserDocuments = dataPath;

    // Convert '\\' to '/'
    replace(appUserDocuments.begin(), appUserDocuments.end(), '\\', '/');

    return CefString(appUserDocuments);
}

CefString AppGetCachePath() {
    std::wstring cachePath = AppGetSupportDirectory();
    cachePath +=  L"/cef_data";

    return CefString(cachePath);
}

int GetInitialUrl(wchar_t* url) {
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = url;
    ofn.nMaxFile = MAX_UNC_PATH;
    ofn.lpstrFilter = L"Web Files\0*.htm;*.html\0\0";
    ofn.lpstrTitle = L"Please select the " APP_NAME L" index.html file.";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

    if (GetOpenFileName(&ofn)) {
        return 0;
    }

    // User cancelled
    return -1;
}

int AppInitInitialUrl(CefRefPtr<CefCommandLine> command_line) {
    static wchar_t url[MAX_UNC_PATH] = {0};

    if (command_line->HasSwitch(client::switches::kStartupPath)) {
        wcscpy(url, command_line->GetSwitchValue(client::switches::kStartupPath).c_str());
    }
    else {
        bool isShiftKeyDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? true : false;

        // If the shift key is not pressed, look for the index.html file 
        if (!isShiftKeyDown) {
            // Get the full pathname for the app. We look for the index.html
            // file relative to this location.
            wchar_t appPath[MAX_UNC_PATH];
            wchar_t *pathRoot;
            GetModuleFileName(NULL, appPath, MAX_UNC_PATH);

            // Strip the .exe filename (and preceding "\") from the appPath
            // and store in pathRoot
            pathRoot = wcsrchr(appPath, '\\');

            // Look for .\dev\src\index.html first
            wcscpy(pathRoot, L"\\dev\\src\\index.html");

            // If the file exists, use it
            if (GetFileAttributes(appPath) != INVALID_FILE_ATTRIBUTES) {
                wcscpy(url, appPath);
            }

            if (!wcslen(url)) {
                // Look for .\www\index.html next
                wcscpy(pathRoot, L"\\www\\index.html");
                if (GetFileAttributes(appPath) != INVALID_FILE_ATTRIBUTES) {
                    wcscpy(url, appPath);
                }
            }
        }
    }

    if (!wcslen(url)) {
        if (GetInitialUrl(url) < 0) {
            return -1;
        }
    }

    wcscpy(szInitialUrl, url);
    return 0;
}

CefString AppGetInitialURL() {
    return szInitialUrl;
}

}  // namespace appshell
