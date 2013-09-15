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
#include "cefclient.h"
#include "cef_popup_window.h"
#include "cef_registry.h"
#include "client_handler.h"
#include "resource.h"
#include "native_menu_model.h"

// Externals
extern HINSTANCE hInst;

// Constants
static wchar_t   gCefWindowClosingPropName[] = L"CLOSING";

cef_popup_window::cef_popup_window(CefRefPtr<CefBrowser> browser) :
    mBrowser(browser)
{
}

cef_popup_window::~cef_popup_window(void)
{
}

// We create these on the heap so destroy them
//  when it is safe to destroy
void cef_popup_window::PostNcDestroy()
{
    cef_host_window::PostNcDestroy();
    delete this;
}

// WM_CLOSE handler
BOOL cef_popup_window::HandleClose()
{
    if (SafeGetCefBrowserHwnd()) {
        HWND browserHwnd = SafeGetCefBrowserHwnd();
        HANDLE closing = ::GetProp(browserHwnd, ::gCefWindowClosingPropName);
    
        if (closing) {
            ::RemoveProp(browserHwnd, ::gCefWindowClosingPropName);
            return FALSE;
        } else {
            DoCommand(FILE_CLOSE_WINDOW, new CloseWindowCommandCallback(mBrowser));
            return TRUE;
        }
    } else {
        return FALSE;
    }
}

// WM_COMMAND handler
BOOL cef_popup_window::HandleCommand(UINT commandId)
{
    switch (commandId)
    {
    case IDM_CLOSE:
        return HandleClose();
    default:
        DoCommand(commandId);
        return TRUE;
    }
}

// Sets the icon on the popup window
void cef_popup_window::InitSystemIcon()
{
    HICON bigIcon = ::LoadIcon(::hInst, MAKEINTRESOURCE(IDI_CEFCLIENT));
    HICON smIcon = ::LoadIcon(::hInst, MAKEINTRESOURCE(IDI_SMALL));
    
    if (bigIcon) {
        SendMessage(WM_SETICON, ICON_BIG, (LPARAM)bigIcon);
    }
    if (smIcon) {
        SendMessage(WM_SETICON, ICON_SMALL, (LPARAM)smIcon);
    }
}

// Subclasses the HWND and initializes the dark drawing code
bool cef_popup_window::SubclassWindow(HWND wnd)
{
     if (cef_host_window::SubclassWindow(wnd)) {
        InitSystemIcon();
        DragAcceptFiles(TRUE);
        GetBrowser()->GetHost()->SetFocus(true);
        return true;
    }
    return false;
}

LRESULT cef_popup_window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
    case WM_CLOSE:
        if (HandleClose())
            return 0L;
        break;
    case WM_COMMAND:
        if (HandleCommand(LOWORD(wParam)))
            return 0L;
        break;
    }

    LRESULT lr = cef_host_window::WindowProc(message, wParam, lParam);

    return lr;
}

const CefRefPtr<CefBrowser> cef_popup_window::GetBrowser()
{ 
    return mBrowser;   
}
