/*************************************************************************
 *
 * ADOBE CONFIDENTIAL
 * ___________________
 *
 *  Copyright 2013 Adobe Systems Incorporated
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of Adobe Systems Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Adobe Systems Incorporated and its
 * suppliers and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Adobe Systems Incorporated.
 **************************************************************************/
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

	wchar_t key[MAX_PATH];
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

