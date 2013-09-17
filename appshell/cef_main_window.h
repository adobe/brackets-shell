#pragma once
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
#include "cef_host_window.h"

// Private Message
#define ID_WM_COPYDATA_SENDOPENFILECOMMAND    (WM_USER+1001)

// This the main window implementation of the host window
class cef_main_window : public cef_host_window
{
public:
    // Construction/Destruction - Public Members
    cef_main_window(void);
    virtual ~cef_main_window(void);

    static HWND FindFirstTopLevelInstance();

    BOOL Create();
    
    void ShowHelp();
    void ShowAbout();

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    // Initalization - Protected Members
    void SaveWindowRestoreRect();
    void LoadWindowRestoreRect(int& left, int& top, int& width, int& height, int& showCmd);
    void RestoreWindowPlacement(int showCmd);

    // Message Handlers
    BOOL HandleEraseBackground();
    BOOL HandleCreate();
    BOOL HandleSetFocus(HWND hLosingFocus);
    BOOL HandleDestroy();
    BOOL HandleClose();
    BOOL HandleSize(BOOL bMinimize);
    BOOL HandleInitMenuPopup(HMENU hMenuPopup);
    BOOL HandleCommand(UINT commandId);
    BOOL HandleExitCommand();
    BOOL HandlePaint();
    BOOL HandleGetMinMaxInfo(LPMINMAXINFO mmi);
    BOOL HandleCopyData(HWND, PCOPYDATASTRUCT lpCopyData);

    // Implementation
    virtual void PostNcDestroy();
    virtual void GetCefBrowserRect(RECT& rect);
    virtual const CefRefPtr<CefBrowser> GetBrowser();
    virtual void DoRepaintClientArea();

    // Find helper
    static BOOL CALLBACK FindSuitableBracketsInstanceHelper(HWND hwnd, LPARAM lParam);
    static LPCWSTR GetBracketsWindowTitleText();

private:
    // Initialization - Private Members
    static ATOM RegisterWndClass();
};

