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

#include <dwmapi.h>

// Libraries
#pragma comment(lib, "dwmapi")


cef_dark_aero_window::cef_dark_aero_window() :
    mReady(false)
{
}

cef_dark_aero_window::~cef_dark_aero_window()
{
}

void cef_dark_aero_window::DoFinalCleanup()
{
    cef_dark_window::DoFinalCleanup();
}

BOOL cef_dark_aero_window::HandleCreate()
{
    RECT rcClient;
    GetWindowRect(&rcClient);

    // Inform application of the frame change.
    SetWindowPos(NULL, 
                 rcClient.left, 
                 rcClient.top,
                 ::RectWidth(rcClient), 
                 ::RectHeight(rcClient),
                 SWP_FRAMECHANGED);

    cef_dark_window::InitDrawingResources();
    mReady = true;
    return TRUE;
}

BOOL cef_dark_aero_window::HandleActivate() 
{
    HRESULT hr = S_OK;
    
    // Extend the frame into the client area.
    MARGINS margins = {0};

    hr = ::DwmExtendFrameIntoClientArea(mWnd, &margins);

    return SUCCEEDED(hr);
}


// WM_NCHITTEST handler
int cef_dark_aero_window::HandleNcHitTest(LPPOINT ptHit)
{
    RECT rectWindow;
    GetWindowRect(&rectWindow);

    if (!::PtInRect(&rectWindow, *ptHit)) 
        return HTNOWHERE;
    
    RECT rectCaption;
    ComputeWindowCaptionRect(rectCaption);
    NonClientToScreen(&rectCaption);

    if (::PtInRect(&rectCaption, *ptHit)) 
        return HTCAPTION;

    RECT rectCloseButton;
    ComputeCloseButtonRect(rectCloseButton);
    NonClientToScreen(&rectCloseButton);

    if (::PtInRect(&rectCloseButton, *ptHit)) 
        return HTCLOSE;

    RECT rectMaximizeButton;
    ComputeMaximizeButtonRect(rectMaximizeButton);
    NonClientToScreen(&rectMaximizeButton);

    if (::PtInRect(&rectMaximizeButton, *ptHit)) 
        return HTMAXBUTTON;

    RECT rectMinimizeButton;
    ComputeMinimizeButtonRect(rectMinimizeButton);
    NonClientToScreen(&rectMinimizeButton);

    if (::PtInRect(&rectMinimizeButton, *ptHit)) 
        return HTMINBUTTON;

    RECT rectSysIcon;
    ComputeWindowIconRect(rectSysIcon);
    NonClientToScreen(&rectSysIcon);

    if (::PtInRect(&rectSysIcon, *ptHit)) 
        return HTSYSMENU;

    if (!IsZoomed()) {

        // Left Border
        if (ptHit->x >= rectWindow.left && ptHit->x <= rectWindow.left + ::GetSystemMetrics (SM_CYFRAME))
        {
            // it's important that we know if the mouse is on a corner so that
            //    the right mouse cursor is displayed
            if (ptHit->y <= rectWindow.top + ::GetSystemMetrics (SM_CYFRAME))
                 return HTTOPLEFT;
 
            if (ptHit->y >= rectWindow.bottom - ::GetSystemMetrics (SM_CYFRAME))
                 return HTBOTTOMLEFT;
 
            return HTLEFT;
        }

        // Right Border
        if (ptHit->x <= rectWindow.right && ptHit->x >= rectWindow.right - ::GetSystemMetrics (SM_CYFRAME)) 
        {
            // it's important that we know if the mouse is on a corner so that
            //    the right mouse cursor is displayed
            if (ptHit->y <= rectWindow.top + ::GetSystemMetrics (SM_CYFRAME))
                return HTTOPRIGHT;
 
            if (ptHit->y >= rectWindow.bottom - ::GetSystemMetrics (SM_CYFRAME))
                return HTBOTTOMRIGHT;
 
            return HTRIGHT;
        }

        // Top and Bottom Borders
        if (ptHit->y <= rectWindow.top + ::GetSystemMetrics (SM_CYFRAME)) 
             return HTTOP;
             
        if (ptHit->y >= rectWindow.bottom - ::GetSystemMetrics (SM_CYFRAME))
             return HTBOTTOM;
    }

    // If it's not in the menu, it's in the caption
    RECT rectMenu;
    ComputeRequiredMenuRect(rectMenu);
    NonClientToScreen(&rectMenu);

    if (::PtInRect(&rectMenu, *ptHit))
        return HTMENU;

    return HTNOWHERE;
}

BOOL cef_dark_aero_window::HandleSysCommand(UINT command)
{
    if ((command & 0xFFF0) != SC_MAXIMIZE)
        return FALSE;

    // Aero windows still get a border when maximized
    //  The border, however, is the size of the unmaximized 
    //  window. To obviate that border we turn off drawing and 
    //  set the size of the window to zero then maximize the window
    //  and redraw the window.  
    //
    // This creates a new problem: the restored window size is now
    //  0 which, when actually restored, is the size returned from 
    //  the WM_GETMINMAXINFO handler. To obviate that problem we get
    //  the window's restore size before maximizing it and reset it after.

    WINDOWPLACEMENT wp;
    ::ZeroMemory(&wp, sizeof (wp));

    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(&wp);

    SetRedraw(FALSE);
    SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE);
    DefaultWindowProc(WM_SYSCOMMAND, command, 0L);
    SetRedraw(TRUE);

    wp.flags            = 0;
    wp.showCmd          = SW_MAXIMIZE;

    wp.ptMinPosition.x  = -1;
    wp.ptMinPosition.y  = -1;
    wp.ptMaxPosition.x  = -1;
    wp.ptMaxPosition.y  = -1;

    // reset the restore size
    SetWindowPlacement(&wp);
    return TRUE;
}



BOOL cef_dark_aero_window::HandlePaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);

    EnforceMenuBackground();

    DoDrawFrame(hdc);
    DoDrawSystemMenuIcon(hdc);
    DoDrawTitlebarText(hdc);
    DoDrawSystemIcons(hdc);
    DoDrawMenuBar(hdc);

    EndPaint(&ps);
    return TRUE;
}

// Computes the Rect where the menu bar is drawn in window coordinates
void cef_dark_aero_window::ComputeMenuBarRect(RECT& rect)
{
    RECT rectClient;
    RECT rectCaption;

    ComputeWindowCaptionRect(rectCaption);
    GetBrowserRect(rectClient);

    rect.top = rectCaption.bottom;
    rect.bottom = rectClient.top + 1;

    rect.left = rectClient.left;
    rect.right = rectClient.right;
}

void cef_dark_aero_window::UpdateMenuBar()
{

    HDC hdc = GetWindowDC();

    RECT rectWindow ;
    ComputeLogicalWindowRect (rectWindow) ;
    
    ::ExcludeClipRect (hdc, rectWindow.left, rectWindow.top, rectWindow.right, rectWindow.bottom);

    RECT rectMenu;
    ComputeMenuBarRect(rectMenu);

    HRGN hrgnUpdate = ::CreateRectRgnIndirect(&rectMenu);

    if (::SelectClipRgn(hdc, hrgnUpdate) != NULLREGION) {
        DoDrawFrame(hdc);   // Draw menu bar background
        DoDrawMenuBar(hdc); // DraW menu items
    }

    ::DeleteObject(hrgnUpdate);

    ReleaseDC(hdc);
}


BOOL cef_dark_aero_window::GetBrowserRect(RECT& rect) const
{
    cef_dark_window::GetBrowserRect(rect);

    rect.top += 48;
    rect.bottom -= 8;
    rect.left += 8;
    rect.right -= 8;

    return TRUE;
}


BOOL cef_dark_aero_window::HandleNcCalcSize(BOOL calcValidRects, NCCALCSIZE_PARAMS* pncsp, LRESULT* lr)
{
    pncsp->rgrc[0].left   = pncsp->rgrc[0].left   + 0;
    pncsp->rgrc[0].top    = pncsp->rgrc[0].top    + 0;
    pncsp->rgrc[0].right  = pncsp->rgrc[0].right  - 0;
    pncsp->rgrc[0].bottom = pncsp->rgrc[0].bottom - 0;

    *lr = 0;
    return TRUE;
}

LRESULT cef_dark_aero_window::DwpCustomFrameProc(UINT message, WPARAM wParam, LPARAM lParam, bool* pfCallDefWindowProc)
{
    LRESULT lr = 0L;

    *pfCallDefWindowProc = (::DwmDefWindowProc(mWnd, message, wParam, lParam, &lr) == 0);
    
    switch (message) {
    case WM_CREATE:
        if (HandleCreate()) {
            *pfCallDefWindowProc = true;
            lr = 0L;
        }
        break;
    case WM_ACTIVATE:
        if (HandleActivate()) {
            *pfCallDefWindowProc = true;
            lr = 0L;
        }
        break;
    case WM_PAINT:
        if (HandlePaint()) {
            *pfCallDefWindowProc = true;
            lr = 0L;
        }
        break;
    case WM_NCCALCSIZE:
        if (cef_dark_aero_window::HandleNcCalcSize((BOOL)(wParam != 0), reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam), &lr)) {
            *pfCallDefWindowProc = false;
        }
        break;
    case WM_NCHITTEST:    
        if (lr == 0) {
            // Handle hit testing in the NCA if not handled by DwmDefWindowProc.

            POINT pt;
            POINTSTOPOINT(pt, lParam);

            lr = HandleNcHitTest(&pt);
    
            if (lr != HTNOWHERE) {
                *pfCallDefWindowProc = false;
            }
        }
        break;
    }


    return lr;
}


// WindowProc handles dispatching of messages and routing back to the base class or to Windows
LRESULT cef_dark_aero_window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool callDefWindowProc = true;

    switch (message) 
    {

    case WM_MEASUREITEM:
        if (HandleMeasureItem((LPMEASUREITEMSTRUCT)lParam))
            return 0L;
        break;
    case WM_DRAWITEM:
        if (HandleDrawItem((LPDRAWITEMSTRUCT)lParam))
            return 0L;
        break;
    case CDW_UPDATEMENU:
        EnforceMenuBackground();
        UpdateMenuBar();
        return 0L;
    case WM_SETICON:
        mWindowIcon = 0;
        break;
    }

    LRESULT lr = DwpCustomFrameProc(message, wParam, lParam, &callDefWindowProc);

    switch(message) {
    case WM_SYSCOMMAND:
        if (HandleSysCommand((UINT)wParam)) 
            return 0L;
        break;
    case WM_ACTIVATE:
        if (mReady) {
            UpdateMenuBar();
        }
        break;
    case WM_MEASUREITEM:
        if (HandleMeasureItem((LPMEASUREITEMSTRUCT)lParam))
            return 0L;
        break;
    case WM_DRAWITEM:
        if (HandleDrawItem((LPDRAWITEMSTRUCT)lParam))
            return 0L;
        break;
    case CDW_UPDATEMENU:
        EnforceMenuBackground();
        UpdateMenuBar();
        return 0L;
    case WM_SETICON:
        mWindowIcon = 0;
        break;
    }

    if (!callDefWindowProc) {
        return lr;
    }


    return cef_window::WindowProc(message, wParam, lParam);;
}
