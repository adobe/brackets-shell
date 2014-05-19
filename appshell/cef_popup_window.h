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

// Wrapper class for secondary windows 
//  makes popup windows dark
class cef_popup_window : public cef_host_window
{
public:
    // Construction/Destruction - Public Members
    cef_popup_window(CefRefPtr<CefBrowser> browser);
    virtual ~cef_popup_window(void);

    bool SubclassWindow(HWND hWnd);

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    // Message Handlers
    BOOL HandleClose();
    BOOL HandleCommand(UINT commandId);
    BOOL HandleSize(BOOL bMinimize);

    // Initialization
    void InitSystemIcon();
    void SetClassStyles();

    // Implementation
    virtual void PostNcDestroy();
    virtual const CefRefPtr<CefBrowser> GetBrowser();

    // Attributes
    CefRefPtr<CefBrowser> mBrowser;
};
