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
#include "client_handler.h"
#include "cef_host_window.h"
#include "native_menu_model.h"

// externals
extern CefRefPtr<ClientHandler> g_handler;


cef_host_window::cef_host_window(void)
{
}
cef_host_window::~cef_host_window(void)
{
}

BOOL cef_host_window::CanUseAeroGlass() const
{
#if defined(DARK_AERO_GLASS) && defined (DARK_UI)
    #if defined(DARK_AERO_GLASS)
        static BOOL bCalled = FALSE;
        static BOOL fDwmEnabled = FALSE;

        if (!bCalled) {
            bCalled = SUCCEEDED(CDwmDLL::DwmIsCompositionEnabled(&fDwmEnabled));
        }

        return fDwmEnabled;
    #else
        return FALSE;
    #endif
#else
    return FALSE;
#endif
}

bool cef_host_window::SubclassWindow(HWND hWnd)
{
#if defined(DARK_AERO_GLASS) && defined (DARK_UI)
    if (CanUseAeroGlass()) {
        return cef_dark_aero_window::SubclassWindow(hWnd);
    }
    else 
#endif
    {
#if defined(DARK_UI)
        return cef_dark_window::SubclassWindow(hWnd);
#else
        return cef_window::SubclassWindow(hWnd);
#endif
    }
}


BOOL cef_host_window::GetBrowserRect(RECT& r) const
{
#if defined(DARK_AERO_GLASS) && defined (DARK_UI)
    if (CanUseAeroGlass()) {
        return cef_dark_aero_window::GetRealClientRect(&r); 
    } 
#endif
    return GetClientRect(&r); 
}

HWND cef_host_window::GetBrowserHwnd()
{
    return GetWindow(GW_CHILD);
}

HWND cef_host_window::SafeGetCefBrowserHwnd()
{
    if (g_handler.get() && GetBrowser().get() && GetBrowser()->GetHost().get())
        return GetBrowser()->GetHost()->GetWindowHandle();
    return NULL;
}

// WM_INITMENUPOPUP handler
BOOL cef_host_window::HandleInitMenuPopup(HMENU hMenuPopup)
{
    // Load the menu item state from the menu state cache
    int count = ::GetMenuItemCount(hMenuPopup);
    void* menuParent = ::getMenuParent(GetBrowser());
    
    for (int i = 0; i < count; i++) {
        UINT id = GetMenuItemID(hMenuPopup, i);

        bool enabled = NativeMenuModel::getInstance(menuParent).isMenuItemEnabled(id);
        UINT flagEnabled = enabled ? MF_ENABLED | MF_BYCOMMAND : MF_DISABLED | MF_GRAYED | MF_BYCOMMAND;
        EnableMenuItem(hMenuPopup, id,  flagEnabled);

        bool checked = NativeMenuModel::getInstance(menuParent).isMenuItemChecked(id);
        UINT flagChecked = checked ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
        CheckMenuItem(hMenuPopup, id, flagChecked);
    }
    return TRUE;
}

// DoCommand Impl with a command string
BOOL cef_host_window::DoCommand(const CefString& commandString, CefRefPtr<CommandCallback> callback/*=0*/)
{
    if (commandString.size() > 0) 
    {
        g_handler->SendJSCommand(GetBrowser(), commandString, callback);
        return TRUE;
    }
    return FALSE;
}

// GetCommandString: CommandID => CommandString
CefString cef_host_window::GetCommandString(UINT commandId)
{
    return NativeMenuModel::getInstance(::getMenuParent(GetBrowser())).getCommandId(commandId);
}

// DoCommand Impl with a command id
BOOL cef_host_window::DoCommand(UINT commandId, CefRefPtr<CommandCallback> callback/*=0*/)
{
    CefString commandString = GetCommandString(commandId);
    if (callback == NULL) {
        callback = new EditCommandCallback(GetBrowser(), commandString);
    }
    return DoCommand(commandString, callback);
}

void cef_host_window::RecomputeLayout()
{
#if defined(DARK_AERO_GLASS) && defined (DARK_UI)
    HWND hWndBrowser = GetBrowserHwnd();
    if (hWndBrowser && mReady && CanUseAeroGlass()) {
        RECT rectBrowser;
        ::SetRectEmpty(&rectBrowser);
        GetBrowserRect(rectBrowser);
        ::MoveWindow(hWndBrowser, rectBrowser.left, rectBrowser.top, ::RectWidth(rectBrowser), ::RectHeight(rectBrowser), FALSE);
    }
#endif
}

// WM_SIZE handler
BOOL cef_host_window::HandleSize(BOOL bMinimize)
{
#ifdef DARK_UI
    // We turn off redraw during activation to minimized flicker
    //    which causes problems on some versions of Windows. If the app
    //  was minimized and was re-activated, it will restore and the client area isn't 
    //    drawn so redraw the client area now or it will be hollow in the middle...
    if (GetProp(L"WasMinimized")) {
        DoRepaintClientArea();
    }
    SetProp(L"WasMinimized", (HANDLE)bMinimize);
#endif
    NotifyWindowMovedOrResized();
    return FALSE;
}

void cef_host_window::NotifyWindowMovedOrResized()
{
    if (GetBrowser() && GetBrowser()->GetHost()) {
        GetBrowser()->GetHost()->NotifyMoveOrResizeStarted();
    }
}

BOOL cef_host_window::HandleMoveOrMoving() 
{
    NotifyWindowMovedOrResized();
    return FALSE;
}

void cef_host_window::DoRepaintClientArea()
{
    CefWindowHandle hwnd = SafeGetCefBrowserHwnd();
    if (!hwnd) 
        return;

    RECT rect;
    GetClientRect(&rect);
    
    ::RedrawWindow(hwnd, &rect, NULL, RDW_ERASE|RDW_INTERNALPAINT|RDW_INVALIDATE|RDW_ERASENOW|RDW_UPDATENOW|RDW_ALLCHILDREN);
}


// Window Proc dispatches window messages
LRESULT cef_host_window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
    case WM_SIZE:
        if (HandleSize(wParam == SIZE_MINIMIZED))
            return 0L;
        break;
    case WM_INITMENUPOPUP:
        if (HandleInitMenuPopup((HMENU)wParam))
            return 0L;
        break;
    case WM_MOVING:
    case WM_MOVE:
        if (HandleMoveOrMoving())
            return 0L;
        break;
    }



    LRESULT lr = 0;
    
#if defined(DARK_AERO_GLASS) && defined (DARK_UI)
    if (CanUseAeroGlass()) {
        lr = cef_dark_aero_window::WindowProc(message, wParam, lParam);
    }
    else 
#endif
    {
#if defined(DARK_UI)
        lr = cef_dark_window::WindowProc(message, wParam, lParam);
#else
        lr = cef_window::WindowProc(message, wParam, lParam);
#endif
    }

    if (message == WM_WINDOWPOSCHANGING) {
        LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;
        if (lpwp->flags & SWP_FRAMECHANGED) {
            RecomputeLayout();
        }
    }


    return lr;
}
