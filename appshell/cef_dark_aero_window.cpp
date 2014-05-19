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
#include <stdio.h>
// Constants
static const int kWindowFrameSize = 8;
static const int kSystemIconZoomFactorCX = kWindowFrameSize + 2;
static const int kSystemIconZoomFactorCY = kWindowFrameSize + 4;

// dll instance to dynamically load the Desktop Window Manager DLL
static CDwmDLL gDesktopWindowManagerDLL;

// DWM Initializers 
HINSTANCE CDwmDLL::mhDwmDll = NULL;
PFNDWMEFICA CDwmDLL::pfnDwmExtendFrameIntoClientArea = NULL;
PFNDWMDWP CDwmDLL::pfnDwmDefWindowProc = NULL;
PFNDWMICE CDwmDLL::pfnDwmIsCompositionEnabled = NULL;

CDwmDLL::CDwmDLL()
{
    LoadLibrary();
}

CDwmDLL::~CDwmDLL()
{
    FreeLibrary();
}

HINSTANCE CDwmDLL::LoadLibrary()
{
    if (mhDwmDll == NULL)
    {
        // dynamically load dwmapi.dll if running Windows Vista or later (ie. not on XP)
        OSVERSIONINFO osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);
        if ((osvi.dwMajorVersion > 5) || ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion >= 1) ))
        {
            mhDwmDll = ::LoadLibrary(TEXT("dwmapi.dll"));
            if (mhDwmDll != NULL)
            {
                pfnDwmExtendFrameIntoClientArea = (PFNDWMEFICA)::GetProcAddress(mhDwmDll, "DwmExtendFrameIntoClientArea");
                pfnDwmDefWindowProc = (PFNDWMDWP)::GetProcAddress(mhDwmDll, "DwmDefWindowProc");
                pfnDwmIsCompositionEnabled = (PFNDWMICE)::GetProcAddress(mhDwmDll, "DwmIsCompositionEnabled");
            }
        }
    }
    return mhDwmDll;
}

void CDwmDLL::FreeLibrary()
{
    if (mhDwmDll != NULL)
    {
        ::FreeLibrary(mhDwmDll);
        mhDwmDll = NULL;
    }
}

HRESULT CDwmDLL::DwmExtendFrameIntoClientArea(HWND hWnd, const MARGINS* pMarInset)
{
    return (pfnDwmExtendFrameIntoClientArea != NULL) ? (*pfnDwmExtendFrameIntoClientArea)(hWnd, pMarInset) : S_FALSE;
}

BOOL CDwmDLL::DwmDefWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    return (pfnDwmDefWindowProc != NULL) ? (*pfnDwmDefWindowProc)(hWnd, msg, wParam, lParam, plResult) : FALSE;
}

HRESULT CDwmDLL::DwmIsCompositionEnabled(BOOL* pfEnabled)
{
    return (pfnDwmIsCompositionEnabled != NULL) ? (*pfnDwmIsCompositionEnabled)(pfEnabled) : S_FALSE;
}


cef_dark_aero_window::cef_dark_aero_window() :
    mReady(false),
    mMenuHiliteIndex(-1),
    mMenuActiveIndex(-1)
{
}

cef_dark_aero_window::~cef_dark_aero_window()
{
}

// WM_CREATE hander
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

    InitDrawingResources();
    mReady = true;
    return TRUE;
}

bool cef_dark_aero_window::SubclassWindow(HWND hWnd)
{
    InitDrawingResources();
    if (cef_window::SubclassWindow(hWnd)) {
        RECT rcClient;
        GetWindowRect(&rcClient);

        // Inform application of the frame change.
        SetWindowPos(NULL, 
                     rcClient.left, 
                     rcClient.top,
                     ::RectWidth(rcClient), 
                     ::RectHeight(rcClient),
                     SWP_FRAMECHANGED);

        mReady = true;
        return TRUE;
    }
    return FALSE;
}

// WM_ACTIVATE handler
BOOL cef_dark_aero_window::HandleActivate() 
{
    HRESULT hr = S_OK;
    
    // Extend the frame into the client area.
    MARGINS margins = {0};

    hr = CDwmDLL::DwmExtendFrameIntoClientArea(mWnd, &margins);

    return SUCCEEDED(hr);
}

// Computes the Rect where the System Icon is drawn in window coordinates
void cef_dark_aero_window::ComputeWindowIconRect(RECT& rect) const
{
	if (CanUseAeroGlass()) {
		int top = ::kWindowFrameSize;
		int left = ::kWindowFrameSize;

		if (IsZoomed()) {
			top = ::kSystemIconZoomFactorCY;
			left = ::kSystemIconZoomFactorCX;    
		}

		::SetRectEmpty(&rect);
		rect.top =  top;
		rect.left = left;
		rect.bottom = rect.top + ::GetSystemMetrics(SM_CYSMICON);
		rect.right = rect.left + ::GetSystemMetrics(SM_CXSMICON);
	} else {
		cef_dark_window::ComputeWindowIconRect(rect);
	}
}

// Computes the Rect where the window caption is drawn in window coordinates
void cef_dark_aero_window::ComputeWindowCaptionRect(RECT& rect) const
{
	if (CanUseAeroGlass()) {
		RECT wr;
		GetWindowRect(&wr);

		rect.top = ::kWindowFrameSize;
		rect.bottom = rect.top + mNcMetrics.iCaptionHeight;

		RECT ir;
		ComputeWindowIconRect(ir);

		RECT mr;
		ComputeMinimizeButtonRect(mr);

		rect.left = ir.right + ::kWindowFrameSize;
		rect.right = mr.left - ::kWindowFrameSize;
	} else {
		cef_dark_window::ComputeWindowCaptionRect(rect);
	}
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
        if (ptHit->x >= rectWindow.left && ptHit->x <= rectWindow.left + ::kWindowFrameSize)
        {
            // it's important that we know if the mouse is on a corner so that
            //    the right mouse cursor is displayed
            if (ptHit->y <= rectWindow.top + ::kWindowFrameSize)
                 return HTTOPLEFT;
 
            if (ptHit->y >= rectWindow.bottom - ::kWindowFrameSize)
                 return HTBOTTOMLEFT;
 
            return HTLEFT;
        }

        // Right Border
        if (ptHit->x <= rectWindow.right && ptHit->x >= rectWindow.right - ::kWindowFrameSize) 
        {
            // it's important that we know if the mouse is on a corner so that
            //    the right mouse cursor is displayed
            if (ptHit->y <= rectWindow.top + ::kWindowFrameSize)
                return HTTOPRIGHT;
 
            if (ptHit->y >= rectWindow.bottom - ::kWindowFrameSize)
                return HTBOTTOMRIGHT;
 
            return HTRIGHT;
        }

        // Top and Bottom Borders
        if (ptHit->y <= rectWindow.top + ::kWindowFrameSize) 
             return HTTOP;
             
        if (ptHit->y >= rectWindow.bottom - ::kWindowFrameSize)
             return HTBOTTOM;
    }

    RECT rectMenu;
    ComputeRequiredMenuRect(rectMenu);
    NonClientToScreen(&rectMenu);

    if (::PtInRect(&rectMenu, *ptHit))
        return HTMENU;

    // If it's in the menu bar but not actually
    //  on a menu item, then return caption so 
    //  the window can be dragged from the dead space 
    ComputeMenuBarRect(rectMenu);
    NonClientToScreen(&rectMenu);
    if (::PtInRect(&rectMenu, *ptHit))
        return HTCAPTION;

    // Aero requires that we return HTNOWHERE
    return HTNOWHERE;
}

// Setup the device context for drawing
void cef_dark_aero_window::InitDeviceContext(HDC hdc)
{
	if (CanUseAeroGlass()) {
        RECT rectClipClient;
        SetRectEmpty(&rectClipClient);
        GetRealClientRect(&rectClipClient);

        // exclude the client area to reduce flicker
        ::ExcludeClipRect(hdc, rectClipClient.left, rectClipClient.top, rectClipClient.right, rectClipClient.bottom);
    } else {
        cef_dark_window::InitDeviceContext(hdc);
    }
}

// Force Drawing the non-client area.
//  Normally WM_NCPAINT is used but there are times when you
//  need to force drawing the entire non-client area when
//  legacy windows message handlers start drawing non-client
//  artifacts over top of us
void cef_dark_aero_window::UpdateNonClientArea()
{
	if (CanUseAeroGlass()) {
        HDC hdc = GetDC();
        DoPaintNonClientArea(hdc);
        ReleaseDC(hdc);
    } else {
        cef_dark_window::UpdateNonClientArea();
    }
}

// WM_PAINT handler
//  since we're extending the client area into the non-client
//  area, we have to draw all of the non-clinet stuff during
//  WM_PAINT
BOOL cef_dark_aero_window::HandlePaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);

    DoPaintNonClientArea(hdc);

    EndPaint(&ps);
    return TRUE;
}

// Computes the Rect where the menu bar is drawn in window coordinates
void cef_dark_aero_window::ComputeMenuBarRect(RECT& rect) const
{
    if (CanUseAeroGlass()) {
        RECT rectClient;
        RECT rectCaption;

        ComputeWindowCaptionRect(rectCaption);
        GetRealClientRect(&rectClient);

        rect.top = ::GetSystemMetrics(SM_CYFRAME) + mNcMetrics.iCaptionHeight + 1;
        rect.bottom = rectClient.top - 1;

        rect.left = rectClient.left;
        rect.right = rectClient.right;
    } else {
        cef_dark_window::ComputeMenuBarRect(rect);
    }
}

void cef_dark_aero_window::DrawMenuBar(HDC hdc)
{
    RECT rectWindow ;
    ComputeLogicalWindowRect (rectWindow) ;
    
    ::ExcludeClipRect (hdc, rectWindow.left, rectWindow.top, rectWindow.right, rectWindow.bottom);

    RECT rectRequired;
    ComputeRequiredMenuRect(rectRequired);

    // No menu == nothing to draw 
    if (::RectHeight(rectRequired) > 0) {
        RECT rectMenu;
        ComputeMenuBarRect(rectMenu);

        HRGN hrgnUpdate = ::CreateRectRgnIndirect(&rectMenu);

        if (::SelectClipRgn(hdc, hrgnUpdate) != NULLREGION) {
	        DoDrawFrame(hdc);   // Draw menu bar background
	        DoDrawMenuBar(hdc); // DraW menu items
        }

        ::DeleteObject(hrgnUpdate);
    }
}

void cef_dark_aero_window::UpdateMenuBar()
{
    cef_buffered_dc dc(this);
    DrawMenuBar(dc);
}

// The Aero version doesn't send us WM_DRAWITEM messages
//  to draw the item when hovering so we have to do that
// Pass NULL for pt to remove the highlight from any menu item
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

// WM_NCMOUSELEAVE handler
void cef_dark_aero_window::HandleNcMouseLeave()
{
    if (mMenuActiveIndex == -1) {
        HiliteMenuItemAt(NULL);
    }
    cef_dark_window::HandleNcMouseLeave();
}

// WM_NCLBUTTONDOWN handler
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

// WM_NCMOUSEMOVE handler
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

// This is a special version of GetClientRect for Aero Glass
//  to give us the portion of the window that is not our custom
//  non-client glass so we can:
//      1) Exclude it from drawing the background and other stuffs
//      2) Position the browser window in derived classes
BOOL cef_dark_aero_window::GetRealClientRect(LPRECT rect) const
{
    GetClientRect(rect);

    RECT rectMenu;
    ComputeRequiredMenuRect(rectMenu);

    RECT rectCaption;
    ComputeWindowCaptionRect(rectCaption);

    rect->top = rectCaption.bottom + ::RectHeight(rectMenu) + 4;
    rect->bottom -= ::kWindowFrameSize;
    rect->left += ::kWindowFrameSize;
    rect->right -= ::kWindowFrameSize;

    return TRUE;
}

// WM_NCCALCSIZE handler.  
//  Basically tells the system that there is no non-client area
BOOL cef_dark_aero_window::HandleNcCalcSize(BOOL calcValidRects, NCCALCSIZE_PARAMS* pncsp, LRESULT* lr)
{
    pncsp->rgrc[0].left   = pncsp->rgrc[0].left   + 0;
    pncsp->rgrc[0].top    = pncsp->rgrc[0].top    + 0;
    pncsp->rgrc[0].right  = pncsp->rgrc[0].right  - 0;
    pncsp->rgrc[0].bottom = pncsp->rgrc[0].bottom - 0;

    *lr = IsZoomed() ? WVR_REDRAW : 0;
    return TRUE;
}

// Helper to dispatch messages to the Desktop Window Manager for processing
LRESULT cef_dark_aero_window::DwpCustomFrameProc(UINT message, WPARAM wParam, LPARAM lParam, bool* pfCallDefWindowProc)
{
    LRESULT lr = 0L;

    *pfCallDefWindowProc = CDwmDLL::DwmDefWindowProc(mWnd, message, wParam, lParam, &lr) == 0;
    
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
        if (HandleNcCalcSize((BOOL)(wParam != 0), reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam), &lr)) {
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
        {
            RECT rectIcon;
            ComputeWindowIconRect(rectIcon);
            InvalidateRect(&rectIcon);
        }
        break;
    }

    // First let the DesktopWindowManager handler the message and tell us if 
    //  we should pass the message to the default window proc
    LRESULT lr = DwpCustomFrameProc(message, wParam, lParam, &callDefWindowProc);

    switch(message) {
    case WM_NCACTIVATE:
    case WM_ACTIVATE:
        if (mReady) {
            UpdateNonClientArea();
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

    // call DefWindowProc?
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
        if (message == WM_WINDOWPOSCHANGED) 
        {
            RECT rect;
            ComputeMenuBarRect(rect);
            InvalidateRect(&rect, TRUE);
        }
        break;
    case WM_EXITMENULOOP:
        mMenuActiveIndex = -1;
        break;   
    case WM_ACTIVATEAPP:
        mIsActive = (BOOL)wParam;
        UpdateNonClientArea();
        break;
    }

    return lr;
}
