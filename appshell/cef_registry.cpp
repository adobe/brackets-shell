/*
 * Copyright (c) 2013 Adobe Systems Incorporated. All rights reserved.
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
 */
#include <windows.h>

#ifndef OS_WIN
#define OS_WIN 1
#endif

#include "config.h"
#include "util.h"

static const wchar_t g_szSoftwareFolder[] = L"Software";


// Add trailing separator, if necessary
void EnsureTrailingSeparator(LPWSTR pRet)
{
    if (!pRet)
        return;

    int len = wcslen(pRet);
    if (len > 0 && wcscmp(&(pRet[len-1]), L"\\") != 0)
    {
        wcscat(pRet, L"\\");
    }
}


// Helper method to build Registry Key string
void GetKey(LPCWSTR pBase, LPCWSTR pGroup, LPCWSTR pApp, LPCWSTR pFolder, LPWSTR pRet)
{
    // Check for required params
    ASSERT(pBase && pApp && pRet);
    if (!pBase || !pApp || !pRet)
        return;

    // Base
    wcscpy(pRet, pBase);

    // Group (optional)
    if (pGroup && (pGroup[0] != '\0'))
    {
        EnsureTrailingSeparator(pRet);
        wcscat(pRet, pGroup);
    }

    // App name
    EnsureTrailingSeparator(pRet);
    wcscat(pRet, pApp);

    // Folder (optional)
    if (pFolder && (pFolder[0] != '\0'))
    {
        EnsureTrailingSeparator(pRet);
        wcscat(pRet, pFolder);
    }
}

// get integer value from registry key
// caller can either use return value to determine success/fail, or pass a default to be used on fail
bool GetRegistryInt(LPCWSTR pFolder, LPCWSTR pEntry, int* pDefault, int& ret)
{
    HKEY hKey;
    bool result = false;

    wchar_t key[MAX_UNC_PATH];
    key[0] = '\0';
    GetKey(g_szSoftwareFolder, GROUP_NAME, APP_NAME, pFolder, (LPWSTR)&key);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, (LPCWSTR)key, 0, KEY_READ, &hKey))
    {
        DWORD dwValue = 0;
        DWORD dwType = 0;
        DWORD dwCount = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, pEntry, NULL, &dwType, (LPBYTE)&dwValue, &dwCount))
        {
            result = true;
            ASSERT(dwType == REG_DWORD);
            ASSERT(dwCount == sizeof(dwValue));
            ret = (int)dwValue;
        }
        RegCloseKey(hKey);
    }

    if (!result)
    {
        // couldn't read value, so use default, if specified
        if (pDefault)
            ret = *pDefault;
    }

    return result;
}

// write integer value to registry key
bool WriteRegistryInt(LPCWSTR pFolder, LPCWSTR pEntry, int val)
{
    HKEY hKey;
    bool result = false;

    wchar_t key[MAX_PATH];
    key[0] = '\0';
    GetKey(g_szSoftwareFolder, GROUP_NAME, APP_NAME, pFolder, (LPWSTR)&key);

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, (LPCWSTR)key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
    {
        DWORD dwCount = sizeof(int);
        if (ERROR_SUCCESS == RegSetValueEx(hKey, pEntry, 0, REG_DWORD, (LPBYTE)&val, dwCount))
            result = true;
        RegCloseKey(hKey);
    }

    return result;
}
