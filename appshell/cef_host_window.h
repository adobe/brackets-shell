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
#include "cef_dark_aero_window.h"
#include "include/cef_browser.h"
#include "command_callbacks.h"
#include "config.h"

// This the base class for all windows which host a browser instance
//  You must derive a from this class and implement GetBrowser()
//  Or use cef_main_window or cef_popup_window which implement GetBrowser()
#ifdef DARK_UI
    #ifdef DARK_AERO_GLASS
        typedef cef_dark_aero_window cef_host_window_base;
    #else
        typedef cef_dark_window cef_host_window_base;
    #endif
#else
    typedef cef_window cef_host_window_base;
#endif

class cef_host_window : public cef_host_window_base
{
public:
    // Construction/Destruciton - Public Members
    cef_host_window(void);
    virtual ~cef_host_window(void);

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    HWND SafeGetCefBrowserHwnd();
    HWND GetBrowserHwnd();
    bool SubclassWindow(HWND hWnd);

    virtual BOOL CanUseAeroGlass() const;

    BOOL GetBrowserRect(RECT& r) const;

protected:
    // Must be impelemented in derived class
    virtual const CefRefPtr<CefBrowser> GetBrowser() = 0;

    void RecomputeLayout();

    // Message Handlers 
    BOOL HandleInitMenuPopup(HMENU hMenuPopup);
    BOOL HandleSize(BOOL bMinimize);
    BOOL HandleMoveOrMoving();

    // Command Implementation
    BOOL DoCommand(UINT commandId, CefRefPtr<CommandCallback> callback = 0);
    BOOL DoCommand(const CefString& commandString, CefRefPtr<CommandCallback> callback = 0);

    // Implementation
    virtual void DoRepaintClientArea();
    void NotifyWindowMovedOrResized();

    // Helper to get a command string from command id
    CefString GetCommandString(UINT commandId);
};
