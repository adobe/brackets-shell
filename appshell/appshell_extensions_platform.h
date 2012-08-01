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

#pragma once

#include "include/cef_browser.h"
#include "include/cef_v8.h"

#include <string>

// Extension error codes. These MUST be in sync with the error
// codes in brackets_extensions.js
#if !defined(OS_WIN) // NO_ERROR is defined on windows
static const int NO_ERROR                   = 0;
#endif
static const int ERR_UNKNOWN                = 1;
static const int ERR_INVALID_PARAMS         = 2;
static const int ERR_NOT_FOUND              = 3;
static const int ERR_CANT_READ              = 4;
static const int ERR_UNSUPPORTED_ENCODING   = 5;
static const int ERR_CANT_WRITE             = 6;
static const int ERR_OUT_OF_SPACE           = 7;
static const int ERR_NOT_FILE               = 8;
static const int ERR_NOT_DIRECTORY          = 9;

#if defined(OS_WIN)
typedef std::wstring ExtensionString;
#else
typedef std::string ExtensionString;
#endif

namespace appshell_extensions {

// Native extension code. These are implemented in appshell_extensions_mac.mm
// and appshell_extensions_win.cpp
int32 OpenLiveBrowser(ExtensionString argURL, bool enableRemoteDebugging);

int32 CloseLiveBrowser(CefRefPtr<CefBrowser> browser);

int32 ShowOpenDialog(bool allowMulitpleSelection,
                     bool chooseDirectory,
                     ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString fileTypes,
                     CefRefPtr<CefListValue>& selectedFiles);

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents);

int32 GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir);

int32 ReadFile(ExtensionString filename, ExtensionString encoding, std::string& contents);

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding);

int32 SetPosixPermissions(ExtensionString filename, int32 mode);

int32 DeleteFileOrDirectory(ExtensionString filename);

} // namespace appshell_extensions

void OnBeforeShutdown();

void CloseWindow(CefRefPtr<CefBrowser> browser);

void BringBrowserWindowToFront(CefRefPtr<CefBrowser> browser);
