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

#include <sstream>
#include <windows.h>

#include "include/cef_version.h"

namespace appshell {

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

}  // namespace appshell
