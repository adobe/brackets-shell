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
// codes in appshell_extensions.js
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
static const int ERR_FILE_EXISTS            = 10;

#if defined(OS_WIN)
typedef std::wstring ExtensionString;
inline void* getMenuParent(CefRefPtr<CefBrowser>browser) {
    if (browser.get() && browser->GetHost()) {
        HWND frameHwnd = browser->GetHost()->GetWindowHandle();
        return (browser->IsPopup()) ? frameHwnd : GetParent(frameHwnd);
    }
    return NULL;
}
#else
typedef std::string ExtensionString;
inline void* getMenuParent(CefRefPtr<CefBrowser>browser) {return NULL;} // Mac uses a shared menu bar
#endif


// Native extension code. These are implemented in appshell_extensions_mac.mm
// and appshell_extensions_win.cpp
int32 OpenLiveBrowser(ExtensionString argURL, bool enableRemoteDebugging);

void CloseLiveBrowser(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response);

int32 OpenURLInDefaultBrowser(ExtensionString url);

int32 ShowOpenDialog(bool allowMulitpleSelection,
                     bool chooseDirectory,
                     ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString fileTypes,
                     CefRefPtr<CefListValue>& selectedFiles);

int32 IsNetworkDrive(ExtensionString path, bool& isRemote);

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents);

int32 MakeDir(ExtensionString path, int32 mode);

int32 Rename(ExtensionString oldName, ExtensionString newName);

int32 GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir);

int32 ReadFile(ExtensionString filename, ExtensionString encoding, std::string& contents);

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding);

int32 SetPosixPermissions(ExtensionString filename, int32 mode);

int32 DeleteFileOrDirectory(ExtensionString filename);

int32 GetNodeState(int32& state);

void OnBeforeShutdown();

void CloseWindow(CefRefPtr<CefBrowser> browser);

void BringBrowserWindowToFront(CefRefPtr<CefBrowser> browser);

int32 ShowFolderInOSWindow(ExtensionString pathname);

int32 GetPendingFilesToOpen(ExtensionString& files);

int32 AddMenu(CefRefPtr<CefBrowser> browser, ExtensionString title, ExtensionString command,
              ExtensionString position, ExtensionString relativeId);

int32 AddMenuItem(CefRefPtr<CefBrowser> browser, ExtensionString parentCommand, ExtensionString itemTitle,
                  ExtensionString command, ExtensionString key, ExtensionString displayStr,
                  ExtensionString position, ExtensionString relativeId);

int32 RemoveMenu(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId);

int32 RemoveMenuItem(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId);

int32 GetMenuItemState(CefRefPtr<CefBrowser> browser, ExtensionString commandId, bool& enabled, bool& checked, int& index);

int32 SetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString menuTitle);

int32 GetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString& menuTitle);

int32 SetMenuItemShortcut(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString shortcut, ExtensionString displayStr);

int32 GetMenuPosition(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId, ExtensionString& parentId, int& index);

void Drag(CefRefPtr<CefBrowser> browser);
