/*
 * Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
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

#include "client_app.h"
#include "resource.h"
#include "include/cef_base.h"
#include "config.h"

#include <algorithm>
#include <MMSystem.h>
#include <ShlObj.h>
#include <string>

extern DWORD g_appStartupTime;

CefString ClientApp::GetCurrentLanguage()
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

std::string ClientApp::GetExtensionJSSource()
{
    extern HINSTANCE hInst;

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

double ClientApp::GetElapsedMilliseconds()
{
    return (timeGetTime() - g_appStartupTime);
}

// returns the directory to which the app has been installed
CefString ClientApp::AppGetAppDirectory()
{
    // find the full pathname of the app .exe
    std::wstring appPath;
    HMODULE hModule = ::GetModuleHandle(NULL);
    if (hModule)
    {
        WCHAR filename[MAX_PATH+1] = {0};
        ::GetModuleFileName(hModule, filename, MAX_PATH);
        appPath = filename;

        // strip off the filename and extension
        int idx = appPath.rfind('\\');
        if (idx >= 0)
            appPath = appPath.substr(0, idx);
    }

    // Convert '\\' to '/'
    replace(appPath.begin(), appPath.end(), '\\', '/');

    return CefString(appPath);
}

CefString ClientApp::AppGetSupportDirectory() 
{
    std::wstring appSupportPath;
    if (!IsPortableInstall())
    {
        // for normal installations, use the user's APPDATA folder
        wchar_t dataPath[MAX_UNC_PATH];
        SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, dataPath);
        appSupportPath = dataPath;
    }
    else
    {
        // for portable installations, use the app's installed folder
        appSupportPath = ClientApp::AppGetAppDirectory();
    }

    appSupportPath +=  L"\\" GROUP_NAME APP_NAME;

    // Convert '\\' to '/'
    replace(appSupportPath.begin(), appSupportPath.end(), '\\', '/');

    return CefString(appSupportPath);
}


CefString ClientApp::AppGetDocumentsDirectory()
{
    wchar_t dataPath[MAX_UNC_PATH] = {0};
    SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, dataPath);
    std::wstring appUserDocuments = dataPath;

    // Convert '\\' to '/'
    replace(appUserDocuments.begin(), appUserDocuments.end(), '\\', '/');

    return CefString(appUserDocuments);
}


// check if this is a portable installation
//   to be a portable installation, the installer should write the empty file to the app folder
bool ClientApp::IsPortableInstall()
{
    typedef enum { UNINITIALIZED = -1, ISNOTPORTABLE, ISPORTABLE } ePortableFlag;
    static ePortableFlag isPortableInstall = UNINITIALIZED;

    if (isPortableInstall == UNINITIALIZED)
    {
	    std::wstring filename;
		GetPortableInstallFilename(filename);
	    HANDLE hFile = ::CreateFile(filename.c_str(), GENERIC_READ,
		    FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	    isPortableInstall = (INVALID_HANDLE_VALUE != hFile) ? ISPORTABLE : ISNOTPORTABLE;
	    ::CloseHandle(hFile);
    }
	return (isPortableInstall == ISPORTABLE);
}

// return the name of the portable install data file
void ClientApp::GetPortableInstallFilename(std::wstring& filename)
{
	// the existence of this file in the same folder as the Brackets application will
	//   cause the application to be run in a "portable" state
	filename = ClientApp::AppGetAppDirectory();
	filename += L"/";
	filename += MAKEPORTABLE_BRACKETS_FILENAME;
}