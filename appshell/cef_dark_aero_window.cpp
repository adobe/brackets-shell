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
    mReady(false),
    mMenuHiliteIndex(-1),
    mMenuActiveIndex(-1)
{
}

cef_dark_aero_window::~cef_dark_aero_window()
{
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

// Setup the device context for drawing
void cef_dark_aero_window::InitDeviceContext(HDC hdc)
{
    RECT rectClipClient;
    SetRectEmpty(&rectClipClient);
    GetRealClientRect(&rectClipClient);

    // exclude the client area to reduce flicker
    ::ExcludeClipRect(hdc, rectClipClient.left, rectClipClient.top, rectClipClient.right, rectClipClient.bottom);

}

void cef_dark_aero_window::DoPaintNonClientArea(HDC hdc)
{
    EnforceMenuBackground();

    HDC hdcOrig = hdc;
    RECT rectWindow;
    GetWindowRect(&rectWindow);

    int Width = ::RectWidth(rectWindow);
    int Height = ::RectHeight(rectWindow);

    HDC dcMem = ::CreateCompatibleDC(hdc);
    HBITMAP bm = ::CreateCompatibleBitmap(hdc, Width, Height);
    HGDIOBJ bmOld = ::SelectObject(dcMem, bm);

    hdc = dcMem;

    InitDeviceContext(hdc);
    InitDeviceContext(hdcOrig);

    DoDrawFrame(hdc);
    DoDrawSystemMenuIcon(hdc);
    DoDrawTitlebarText(hdc);
    DoDrawSystemIcons(hdc);
    DoDrawMenuBar(hdc);

    ::BitBlt(hdcOrig, 0, 0, Width, Height, dcMem, 0, 0, SRCCOPY);

    ::SelectObject(dcMem, bmOld);
    ::DeleteObject(bm);
    ::DeleteDC(dcMem);

}

// Force Drawing the non-client area.
//  Normally WM_NCPAINT is used but there are times when you
//  need to force drawing the entire non-client area when
//  legacy windows message handlers start drawing non-client
//  artifacts over top of us
void cef_dark_aero_window::UpdateNonClientArea()
{
    HDC hdc = GetDC();
    DoPaintNonClientArea(hdc);
    ReleaseDC(hdc);
}

void cef_dark_aero_window::UpdateNonClientButtons () 
{
    // create a simple clipping region
    //  that only includes the system buttons (min/max/restore/close)
    HDC hdc = GetDC();

    RECT rectCloseButton ;
    ComputeCloseButtonRect (rectCloseButton) ;
 
    RECT rectMaximizeButton ;
    ComputeMaximizeButtonRect (rectMaximizeButton) ;
 
    RECT rectMinimizeButton ;
    ComputeMinimizeButtonRect (rectMinimizeButton) ;

    RECT rectWindow ;
    ComputeLogicalWindowRect (rectWindow) ;
    ::ExcludeClipRect (hdc, rectWindow.left, rectWindow.top, rectWindow.right, rectWindow.bottom);

    RECT rectButtons;
    rectButtons.top = rectCloseButton.top;
    rectButtons.right = rectCloseButton.right;
    rectButtons.bottom = rectCloseButton.bottom;
    rectButtons.left = rectMinimizeButton.left;

    HRGN hrgnUpdate = ::CreateRectRgnIndirect(&rectButtons);

    if (::SelectClipRgn(hdc, hrgnUpdate) != NULLREGION) {
        DoDrawSystemIcons(hdc);
    }

    ::DeleteObject(hrgnUpdate);
    ReleaseDC(hdc);
}

BOOL cef_dark_aero_window::HandlePaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);

    DoPaintNonClientArea(hdc);

    EndPaint(&ps);
    return TRUE;
}

// Computes the Rect where the menu bar is drawn in window coordinates
void cef_dark_aero_window::ComputeMenuBarRect(RECT& rect)
{
    RECT rectClient;
    RECT rectCaption;

    ComputeWindowCaptionRect(rectCaption);
    GetRealClientRect(&rectClient);

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

void cef_dark_aero_window::HiliteMenuItemAt(LPPOINT pt)
{
    HDC hdc = GetDC();
    HMENU menu = GetMenu();
    int items = ::GetMenuItemCount(menu);
    int oldMenuHilite = mMenuHiliteIndex;
    
    // reset this in case we don't get a hit below
    mMenuHiliteIndex = -1;

    for (int i = 0; i < items; i++) {
        // Determine the menu item state and ID
        MENUITEMINFO mmi = {0};
        mmi.cbSize = sizeof (mmi);
        mmi.fMask = MIIM_STATE|MIIM_ID;
        ::GetMenuItemInfo (menu, i, TRUE, &mmi);
        
        // Drawitem only works on ID
        MEASUREITEMSTRUCT mis = {0};
        mis.CtlType = ODT_MENU;
        mis.itemID = mmi.wID;

        RECT itemRect;
        ::SetRectEmpty(&itemRect);
        if (::GetMenuItemRect(mWnd, menu, (UINT)i, &itemRect)) {
            bool needsUpdate = false;

            if ((pt) && (::PtInRect(&itemRect, *pt))) {
                mmi.fState |= MFS_HILITE;
                mMenuHiliteIndex = i;
                needsUpdate = true;
            } else if (i == oldMenuHilite) {
                mmi.fState &= ~MFS_HILITE;
                needsUpdate = true;
            }

            if (needsUpdate) {
                // Draw the menu item
                DRAWITEMSTRUCT dis = {0};
                dis.CtlType = ODT_MENU;
                dis.itemID = mmi.wID;
                dis.hwndItem = (HWND)menu;
                dis.itemAction = ODA_DRAWENTIRE;
                dis.hDC = hdc;

               ScreenToNonClient(&itemRect);
                ::CopyRect(&dis.rcItem, &itemRect);

                if (mmi.fState & MFS_HILITE) {
                    dis.itemState |= ODS_SELECTED;
                } 
                if (mmi.fState & MFS_GRAYED) {
                    dis.itemState |= ODS_GRAYED;
                } 

                dis.itemState |= ODS_NOACCEL;

                HandleDrawItem(&dis);
            }
        }
    }

    ReleaseDC(hdc);

}

void cef_dark_aero_window::HandleNcMouseLeave()
{
    if (mMenuActiveIndex == -1) {
        HiliteMenuItemAt(NULL);
    }
    cef_dark_window::HandleNcMouseLeave();
}

BOOL cef_dark_aero_window::HandleNcLeftButtonDown(UINT uHitTest, LPPOINT pt)
{
    if (!cef_dark_window::HandleNcLeftButtonDown(uHitTest)) {
        if (uHitTest == HTMENU) {
            mMenuActiveIndex = mMenuHiliteIndex;
        }
        return FALSE;
    } else {
        return TRUE;
    }
}


BOOL cef_dark_aero_window::HandleNcMouseMove(UINT uHitTest, LPPOINT pt)
{
    if (cef_dark_window::HandleNcMouseMove(uHitTest)) {
        if (mMenuActiveIndex == -1) {
            HiliteMenuItemAt(NULL);
        }
        return TRUE;
    }

    if (mMenuActiveIndex == -1) {
        if (uHitTest == HTMENU) {
            HiliteMenuItemAt(pt);
            TrackNonClientMouseEvents();
        } else {
            HiliteMenuItemAt(NULL);
        }
    }

    return FALSE;
}

BOOL cef_dark_aero_window::GetRealClientRect(LPRECT rect) const
{
    GetClientRect(rect);

    rect->top += 48;
    rect->bottom -= 8;
    rect->left += 8;
    rect->right -= 8;

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
    case WM_NCMOUSELEAVE:
        // NOTE: We want anyone else interested in this message
        //          to be notified. Otherwise the default implementation 
        //          may be in the wrong state
        HandleNcMouseLeave();
        break;
    case WM_NCMOUSEMOVE:
        {
            POINT pt;
            POINTSTOPOINT(pt, lParam);

            if (HandleNcMouseMove((UINT)wParam, &pt))
                return 0L;
        }
        break;
    case WM_NCLBUTTONDOWN:
        {
            POINT pt;
            POINTSTOPOINT(pt, lParam);
            if (HandleNcLeftButtonDown((UINT)wParam, &pt))
                return 0L;
        }
        break;
    case WM_NCLBUTTONUP:
        {
            POINT pt;
            POINTSTOPOINT(pt, lParam);
            if (HandleNcLeftButtonUp((UINT)wParam, &pt))
                return 0L;
        }
        break;
    }

    if (!callDefWindowProc) {
        return lr;
    }


    lr = cef_window::WindowProc(message, wParam, lParam);

    if (!mReady)
        return lr;

    switch (message)
    {
    case WM_SETTEXT:
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    case WM_MOVE:
    case WM_SIZE:
    case WM_SIZING:
    case WM_EXITSIZEMOVE:
        UpdateNonClientArea();
        break;
    case WM_EXITMENULOOP:
        mMenuActiveIndex = -1;
        break;   
    }

    return lr;
}
