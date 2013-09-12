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
#include "cef_dark_window.h"
#include "resource.h"
#include <minmax.h>
#include <objidl.h>
#include <GdiPlus.h>
#include <Uxtheme.h>
#include <Shlwapi.h>

#pragma comment(lib, "gdiplus")
#pragma comment(lib, "UxTheme")

extern HINSTANCE hInst;

static ULONG_PTR gdiplusToken = NULL;

static const int kMenuItemSpacingCX = 4;
static const int kMenuItemSpacingCY = 4;
static const int kMenuFrameThreshholdCX = 12;

static const int kWindowFrameZoomFactorCX = 12;
static const int kWindowFrameZoomFactorCY = 12;

static void RECT2Rect(Gdiplus::Rect& dest, const RECT& src) {
    dest.X = src.left;
    dest.Y = src.top;   
    dest.Width = ::RectWidth(src);
    dest.Height = ::RectHeight(src);
}

namespace ResourceImage
{
    Gdiplus::Image* FromResource(LPCWSTR lpResourceName)
    {
        HRSRC hResource = ::FindResource(::hInst, lpResourceName, L"PNG");
        if (!hResource) 
            return NULL;

        HGLOBAL hData = ::LoadResource(::hInst, hResource);
        if (!hData)
            return NULL;

        void* data = ::LockResource(hData);
        ULONG size = ::SizeofResource(::hInst, hResource);

        IStream* pStream = ::SHCreateMemStream((BYTE*)data, (UINT)size);

        if (!pStream) 
            return NULL;

        Gdiplus::Image* image = Gdiplus::Image::FromStream(pStream);
        pStream->Release();
        return image;
    }

}

cef_dark_window::cef_dark_window() :
    mSysCloseButton(0),
    mSysRestoreButton(0),
    mSysMinimizeButton(0),
    mSysMaximizeButton(0),
    mHoverSysCloseButton(0),
    mHoverSysRestoreButton(0),
    mHoverSysMinimizeButton(0),
    mHoverSysMaximizeButton(0),
    mWindowIcon(0),
    mBackgroundBrush(0),
    mCaptionFont(0),
    mMenuFont(0),
    mHighlightBrush(0),
    mHoverBrush(0)
{
    ::ZeroMemory(&mNcMetrics, sizeof(mNcMetrics));
}

cef_dark_window::~cef_dark_window()
{

}

void cef_dark_window::DoFinalCleanup()
{
    if (gdiplusToken != NULL) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
    }
}


void cef_dark_window::InitDrawingResources()
{
    mNcMetrics.cbSize = sizeof (mNcMetrics);
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &mNcMetrics, 0);

    if (gdiplusToken == NULL) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }

    LoadSysButtonImages();

    ::SetWindowTheme(mWnd, L"", L"");

    if (mBackgroundBrush == NULL) {                            
        mBackgroundBrush = ::CreateSolidBrush(CEF_COLOR_BACKGROUND);
    }
    if (mBackgroundBrush == NULL) {                            
        mBackgroundBrush = ::CreateSolidBrush(CEF_COLOR_BACKGROUND);
    }
    if (mHighlightBrush == NULL) {                            
        mHighlightBrush = ::CreateSolidBrush(CEF_COLOR_MENU_HILITE_BACKGROUND);
    }
    if (mHoverBrush == NULL) {                            
        mHoverBrush = ::CreateSolidBrush(CEF_COLOR_MENU_HOVER_BACKGROUND);
    }
}

void cef_dark_window::InitMenuFont()
{
    if (mMenuFont == NULL) {
        mMenuFont = ::CreateFontIndirect(&mNcMetrics.lfMenuFont);
    }
}

void cef_dark_window::LoadSysButtonImages()
{
    if (mSysCloseButton == NULL) {
        mSysCloseButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_CLOSE_BUTTON));
    }
    if (mSysMaximizeButton == NULL) {
        mSysMaximizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MAX_BUTTON));
    }
    if (mSysMinimizeButton == NULL) {
        mSysMinimizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MIN_BUTTON));
    }
    if (mSysRestoreButton == NULL) {
        mSysRestoreButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_RESTORE_BUTTON));
    }
    if (mHoverSysCloseButton == NULL) {
        mHoverSysCloseButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_CLOSE_HOVER_BUTTON));
    }
    if (mHoverSysRestoreButton == NULL) {
        mHoverSysRestoreButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_RESTORE_HOVER_BUTTON));
    }
    if (mHoverSysMinimizeButton == NULL) {
        mHoverSysMinimizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MIN_HOVER_BUTTON));
    }
    if (mHoverSysMaximizeButton == NULL) {
        mHoverSysMaximizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MAX_HOVER_BUTTON));
    }
}

BOOL cef_dark_window::HandleNcCreate()
{
    InitDrawingResources();
    return FALSE;
}

BOOL cef_dark_window::HandleNcDestroy()
{
	TrackNonClientMouseEvents(false);

    delete mSysCloseButton;
    delete mSysRestoreButton;
    delete mSysMinimizeButton;
    delete mSysMaximizeButton;

    delete mHoverSysCloseButton;
    delete mHoverSysRestoreButton;
    delete mHoverSysMinimizeButton;
    delete mHoverSysMaximizeButton;

    ::DeleteObject(mBackgroundBrush);
    ::DeleteObject(mCaptionFont);
    ::DeleteObject(mMenuFont);
    ::DeleteObject(mHighlightBrush);
    ::DeleteObject(mHoverBrush);

    return cef_window::HandleNcDestroy();
}

BOOL cef_dark_window::HandleSettingChange(UINT uFlags, LPCWSTR lpszSection)
{
    switch (uFlags) {
    case SPI_SETICONTITLELOGFONT:
    case SPI_SETHIGHCONTRAST:
    case SPI_SETNONCLIENTMETRICS:
    case SPI_SETFONTSMOOTHING:
    case SPI_SETFONTSMOOTHINGTYPE:
    case SPI_SETFONTSMOOTHINGCONTRAST:
    case SPI_SETFONTSMOOTHINGORIENTATION:
        mNonClientData.Reset();

        ::DeleteObject(mCaptionFont);
        mCaptionFont = NULL;

        ::DeleteObject(mMenuFont);
        mMenuFont = NULL;

        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &mNcMetrics, 0);
        break;
    }
    return FALSE;
}


void cef_dark_window::ComputeWindowIconRect(RECT& rect)
{
    int top = mNcMetrics.iBorderWidth;
    int left = mNcMetrics.iBorderWidth;

    if (IsZoomed()) {
        top = ::kWindowFrameZoomFactorCX;
        left = ::kWindowFrameZoomFactorCY;
    }

    ::SetRectEmpty(&rect);
    rect.top =  top;
    rect.left = left;
    rect.bottom = rect.top + ::GetSystemMetrics(SM_CYSMICON);
    rect.right = rect.left + ::GetSystemMetrics(SM_CXSMICON);
}

void cef_dark_window::ComputeWindowCaptionRect(RECT& rect)
{
    RECT wr;
    GetWindowRect(&wr);

    int top = mNcMetrics.iBorderWidth;
    int left = mNcMetrics.iBorderWidth;

    if (IsZoomed()) {
        top = 8;
        left = 8;
    }

    rect.top = top;
    rect.bottom = rect.top + ::GetSystemMetrics (SM_CYCAPTION);

    RECT ir;
    ComputeWindowIconRect(ir);

    RECT mr;
    ComputeMinimizeButtonRect(mr);

    rect.left = ir.right + 1;
    rect.right = mr.left - 1;
}

void cef_dark_window::ComputeMinimizeButtonRect(RECT& rect)
{
    ComputeMaximizeButtonRect(rect);

    rect.left -= (mSysMinimizeButton->GetWidth() + 1);
    rect.right = rect.left + mSysMinimizeButton->GetWidth();
    rect.bottom = rect.top + mSysMinimizeButton->GetHeight();
}

void cef_dark_window::ComputeMaximizeButtonRect(RECT& rect)
{
    ComputeCloseButtonRect(rect);

    rect.left -= (mSysMaximizeButton->GetWidth() + 1);
    rect.right = rect.left + mSysMaximizeButton->GetWidth();
    rect.bottom = rect.top + mSysMaximizeButton->GetHeight();
}

void cef_dark_window::ComputeCloseButtonRect(RECT& rect)
{
    int top = 1;
    int right =  mNcMetrics.iBorderWidth;


    if (IsZoomed()) {
        top =  8;
        right = 8;
    }

    RECT wr;
    GetWindowRect(&wr);

    rect.left = ::RectWidth(wr) - right - mSysCloseButton->GetWidth();
    rect.top = top;
    rect.right = rect.left + mSysCloseButton->GetWidth();
    rect.bottom = rect.top + mSysCloseButton->GetHeight();
}

void cef_dark_window::ComputeMenuBarRect(RECT& rect)
{
    RECT rectClient;
    RECT rectCaption;

    ComputeWindowCaptionRect(rectCaption);
    ComputeLogicalClientRect(rectClient);

    rect.top = rectCaption.bottom + 1;
    rect.bottom = rectClient.top - 1;

    rect.left = rectClient.left;
    rect.right = rectClient.right;
}

void cef_dark_window::DoDrawFrame(HDC hdc)
{
    RECT rectWindow;
    GetWindowRect(&rectWindow);

    RECT rectFrame;

    ::SetRectEmpty(&rectFrame);

    rectFrame.bottom = ::RectHeight(rectWindow);
    rectFrame.right = ::RectWidth(rectWindow);

    ::FillRect(hdc, &rectFrame, mBackgroundBrush);
}

void cef_dark_window::DoDrawSystemMenuIcon(HDC hdc)
{
    if (mWindowIcon == 0) {
        mWindowIcon = (HICON)SendMessage(WM_GETICON, ICON_SMALL, 0);

        if (!mWindowIcon) 
            mWindowIcon = (HICON)SendMessage(WM_GETICON, ICON_BIG, 0);

        if (!mWindowIcon)
            mWindowIcon = reinterpret_cast<HICON>(GetClassLongPtr(GCLP_HICONSM));
        
        if (!mWindowIcon)
            mWindowIcon = reinterpret_cast<HICON>(GetClassLongPtr(GCLP_HICON));;

        if (!mWindowIcon) 
            mWindowIcon = ::LoadIcon(NULL, IDI_APPLICATION);
    }

    RECT rectIcon;
    ComputeWindowIconRect(rectIcon);

    ::DrawIconEx(hdc, rectIcon.top, rectIcon.left, mWindowIcon, ::RectWidth(rectIcon), ::RectHeight(rectIcon), 0, NULL, DI_NORMAL);
}

void cef_dark_window::DoDrawTitlebarText(HDC hdc)
{
    if (mCaptionFont == 0) {
        mCaptionFont = ::CreateFontIndirect(&mNcMetrics.lfCaptionFont);
    }

    HGDIOBJ hPreviousFont = ::SelectObject(hdc, mCaptionFont);        

    int oldBkMode = ::SetBkMode(hdc, TRANSPARENT);
    COLORREF oldRGB = ::SetTextColor(hdc, CEF_COLOR_NORMALTEXT);

    RECT textRect;
    ComputeWindowCaptionRect(textRect);

    int textLength = GetWindowTextLength() + 1;
    LPWSTR szCaption = new wchar_t [textLength + 1];
    ::ZeroMemory(szCaption, textLength + 1);
    int cchCaption = GetWindowText(szCaption, textLength);

    DrawText(hdc, szCaption, cchCaption, &textRect, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_END_ELLIPSIS|DT_NOPREFIX);

    delete []szCaption;
    ::SetTextColor(hdc, oldRGB);
    ::SetBkMode(hdc, oldBkMode);
    ::SelectObject(hdc, hPreviousFont);
}

void cef_dark_window::DoDrawSystemIcons(HDC hdc)
{
    Gdiplus::Image* CloseButton = mSysCloseButton;
    Gdiplus::Image* RestoreButton = mSysRestoreButton;
    Gdiplus::Image* MinimizeButton = mSysMinimizeButton;
    Gdiplus::Image* MaximizeButton = mSysMaximizeButton;

    switch ( mNonClientData.mActiveButton )
 	{
 	case HTCLOSE:				
 		CloseButton = mHoverSysCloseButton;
 		break;
 
 	case HTMAXBUTTON:
 		RestoreButton = mHoverSysRestoreButton;
        MaximizeButton = mHoverSysMaximizeButton;
 		break;
 
 	case HTMINBUTTON:
        MinimizeButton = mHoverSysMinimizeButton;
        break ;
 	}

    RECT rcButton;
    Gdiplus::Rect rect;
    Gdiplus::Graphics grpx(hdc);
   
    ComputeCloseButtonRect(rcButton);
    ::RECT2Rect(rect, rcButton);

    grpx.DrawImage(CloseButton, rect);

    ComputeMaximizeButtonRect(rcButton);
    ::RECT2Rect(rect, rcButton);

    if (IsZoomed()) {
        grpx.DrawImage(RestoreButton, rect);
    } else {
        grpx.DrawImage(MaximizeButton, rect);
    }

    ComputeMinimizeButtonRect(rcButton);
    ::RECT2Rect(rect, rcButton);

    grpx.DrawImage(MinimizeButton, rect);
}

void cef_dark_window::EnforceOwnerDrawnMenus()
{
    HMENU hm = GetMenu();
    int items = ::GetMenuItemCount(hm);
    for (int i = 0; i < items; i++) {
        MENUITEMINFO mmi = {0};
        mmi.cbSize = sizeof (mmi);
        mmi.fMask = MIIM_FTYPE;

        ::GetMenuItemInfo(hm, i, TRUE, &mmi);
        if ((mmi.fType & MFT_OWNERDRAW) == 0) {
            mmi.fType |= MFT_OWNERDRAW;
            ::SetMenuItemInfo(hm, i, TRUE, &mmi);
        }
    }
}

void cef_dark_window::EnforceMenuBackground()
{
    MENUBARINFO mbi = {0};
    mbi.cbSize = sizeof(mbi);
    
    GetMenuBarInfo(OBJID_MENU, 0, &mbi);

    MENUINFO mi = {0};
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_BACKGROUND;

    ::GetMenuInfo(mbi.hMenu, &mi);
    
    if (mi.hbrBack != mBackgroundBrush) {
        mi.hbrBack = mBackgroundBrush;
        ::SetMenuInfo(mbi.hMenu, &mi);
    }
}

void cef_dark_window::InitDeviceContext(HDC hdc)
{
    RECT rectClipClient;
    SetRectEmpty(&rectClipClient);
    ComputeLogicalClientRect(rectClipClient);

    // exclude the client area to reduce flicker
    ::ExcludeClipRect(hdc, rectClipClient.left, rectClipClient.top, rectClipClient.right, rectClipClient.bottom);
}

void cef_dark_window::DoDrawMenuBar(HDC hdc)
{
    RECT rectBar;
    ComputeMenuBarRect(rectBar);

    HMENU menu = GetMenu();
    int items = ::GetMenuItemCount(menu);
    
    int i,
        currentTop = rectBar.top + 1,
        currentLeft = rectBar.left;

    for (i = 0; i < items; i++) {
        // Determine the menu item state and ID
        MENUITEMINFO mmi = {0};
        mmi.cbSize = sizeof (mmi);
        mmi.fMask = MIIM_STATE|MIIM_ID;
        ::GetMenuItemInfo (menu, i, TRUE, &mmi);
        
        // Drawitem only works on ID
        MEASUREITEMSTRUCT mis = {0};
        mis.CtlType = ODT_MENU;
        mis.itemID = mmi.wID;

        HandleMeasureItem(&mis);

        RECT itemRect;

        // Compute the rect to draw the item in
        itemRect.top = currentTop + 1;
        itemRect.left = currentLeft + 1 + ::kMenuItemSpacingCX;
        itemRect.right = itemRect.left + mis.itemWidth + ::kMenuItemSpacingCX;
        itemRect.bottom = itemRect.top + mis.itemHeight;

        // check to see if if we need to wrap to a new line
        if (rectBar.left < currentLeft) {
            if (itemRect.right >= (rectBar.right - ::kMenuFrameThreshholdCX)) {
                currentLeft = rectBar.left;
                currentTop = itemRect.bottom - 1;

                itemRect.top = currentTop + 1;
                itemRect.left = currentLeft + 1 + ::kMenuItemSpacingCX;
                itemRect.right = itemRect.left + mis.itemWidth + ::kMenuItemSpacingCX;
                itemRect.bottom = itemRect.top + mis.itemHeight;
            }
        }

        currentLeft = itemRect.right + 1 + ::kMenuItemSpacingCX;

        // Draw the menu item
        DRAWITEMSTRUCT dis = {0};
        dis.CtlType = ODT_MENU;
        dis.itemID = mmi.wID;
        dis.hwndItem = (HWND)menu;
        dis.itemAction = ODA_DRAWENTIRE;
        dis.hDC = hdc;
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


void cef_dark_window::DoPaintNonClientArea(HDC hdc)
{
    EnforceMenuBackground();
    EnforceOwnerDrawnMenus();

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

void cef_dark_window::UpdateNonClientArea()
{
    HDC hdc = GetWindowDC();
    DoPaintNonClientArea(hdc);
    ReleaseDC(hdc);
}

BOOL cef_dark_window::HandleNcPaint(HRGN hUpdateRegion)
{
    HDC hdc = GetDCEx(hUpdateRegion, DCX_WINDOW|DCX_INTERSECTRGN|DCX_USESTYLE);
    DoPaintNonClientArea(hdc);
    ReleaseDC(hdc);

    return TRUE;
}

BOOL cef_dark_window::HandleSysCommand(UINT uType)
{
    return TRUE;
}

int cef_dark_window::HandleNcHitTest(LPPOINT ptHit)
{
    RECT rectWindow;
    GetWindowRect(&rectWindow);

    if (!::PtInRect(&rectWindow, *ptHit)) 
        return HTNOWHERE;
    
    RECT rectClient;
    GetClientRect(&rectClient);
    ClientToScreen(&rectClient);

    if (::PtInRect(&rectClient, *ptHit)) 
        return HTCLIENT;

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

	// Left Border
	if ( ptHit->x >= rectWindow.left && ptHit->x <= rectWindow.left + ::GetSystemMetrics (SM_CYFRAME))
 	{
		// it's important that we know if the mouse is on a corner so that
		//	the right mouse cursor is displayed
		if ( ptHit->y <= rectWindow.top + ::GetSystemMetrics (SM_CYFRAME))
 			return HTTOPLEFT;
 
		if ( ptHit->y >= rectWindow.bottom - ::GetSystemMetrics (SM_CYFRAME) )
 			return HTBOTTOMLEFT;
 
 		return HTLEFT;
 	}

	// Right Border
	if ( ptHit->x <= rectWindow.right && ptHit->x >= rectWindow.right - ::GetSystemMetrics (SM_CYFRAME)) 
 	{
		// it's important that we know if the mouse is on a corner so that
		//	the right mouse cursor is displayed
		if ( ptHit->y <= rectWindow.top + ::GetSystemMetrics (SM_CYFRAME))
 			return HTTOPRIGHT;
 
		if ( ptHit->y >= rectWindow.bottom - ::GetSystemMetrics (SM_CYFRAME))
 			return HTBOTTOMRIGHT;
 
 		return HTRIGHT;
 	}

	// Top and Bottom Borders
 	if ( ptHit->y <= rectWindow.top + ::GetSystemMetrics (SM_CYFRAME)) 
 		return HTTOP;
 			
	if ( ptHit->y >= rectWindow.bottom - ::GetSystemMetrics (SM_CYFRAME))
 		return HTBOTTOM;

	return HTMENU;
}

void cef_dark_window::UpdateNonClientButtons () 
{
    HDC hdc = GetWindowDC();

 	RECT rectCloseButton ;
 	ComputeCloseButtonRect (rectCloseButton) ;
 
 	RECT rectMaximizeButton ;
 	ComputeMaximizeButtonRect ( rectMaximizeButton ) ;
 
 	RECT rectMinimizeButton ;
 	ComputeMinimizeButtonRect ( rectMinimizeButton ) ;

	RECT rectWindow ;
	ComputeLogicalWindowRect ( rectWindow ) ;
    ::ExcludeClipRect (hdc, rectWindow.left, rectWindow.top, rectWindow.right, rectWindow.bottom);

    RECT rectButtons;
    rectButtons.top = rectCloseButton.top;
    rectButtons.right = rectCloseButton.right;
    rectButtons.bottom = rectCloseButton.bottom;
    rectButtons.left = rectMinimizeButton.left;

    HRGN hrgnUpdate = ::CreateRectRgnIndirect(&rectButtons);

    if (::SelectClipRgn(hdc, hrgnUpdate) != NULLREGION) {
        DoPaintNonClientArea(hdc);
    }

    ::DeleteObject(hrgnUpdate);
    ReleaseDC(hdc);
}


BOOL cef_dark_window::HandleMeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
    static wchar_t szMenuString[256] = L"";

    if (lpMIS->CtlType == ODT_MENU) {
        HDC dc      = GetWindowDC();
        HMENU menu  = GetMenu();
        int items   = ::GetMenuItemCount(menu);
        
        InitMenuFont();
        
        HGDIOBJ fontOld = ::SelectObject(dc, mMenuFont);            

        ::GetMenuString(menu, lpMIS->itemID, szMenuString, _countof(szMenuString), MF_BYCOMMAND);

        RECT rectTemp;
        SetRectEmpty(&rectTemp);

        // Calc the size of this menu item 
        ::DrawText(dc, szMenuString, ::wcslen(szMenuString), &rectTemp, DT_SINGLELINE|DT_CALCRECT);

        lpMIS->itemHeight = ::RectHeight(rectTemp) + ::kMenuItemSpacingCY;
        lpMIS->itemWidth = ::RectWidth(rectTemp) + ::kMenuItemSpacingCX;

        ::SelectObject(dc, fontOld);            
        ReleaseDC(dc);
        return TRUE;
    }

    return FALSE;
}


BOOL cef_dark_window::HandleDrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    static wchar_t szMenuString[256] = L"";

    if (lpDIS->CtlType == ODT_MENU) {
        int items = ::GetMenuItemCount((HMENU)lpDIS->hwndItem);

        InitMenuFont();

        HGDIOBJ fontOld = ::SelectObject(lpDIS->hDC, mMenuFont);            
        COLORREF rgbMenuText =  CEF_COLOR_NORMALTEXT;

        ::GetMenuString((HMENU)lpDIS->hwndItem, lpDIS->itemID, szMenuString, _countof(szMenuString), MF_BYCOMMAND);
        
        if (lpDIS->itemState & ODS_SELECTED) {
            ::FillRect(lpDIS->hDC, &lpDIS->rcItem, mHighlightBrush);
            rgbMenuText = CEF_COLOR_MENU_SELECTED_TEXT;
        } else if (lpDIS->itemState & ODS_HOTLIGHT) {
            ::FillRect(lpDIS->hDC, &lpDIS->rcItem, mHoverBrush);
        } else {
            ::FillRect(lpDIS->hDC, &lpDIS->rcItem, mBackgroundBrush);
        }
        
        if (lpDIS->itemState & ODS_GRAYED) {
            rgbMenuText = CEF_COLOR_MENU_DISABLED_TEXT;
        }

        COLORREF oldRGB = ::SetTextColor(lpDIS->hDC, rgbMenuText);
        
        UINT format = DT_CENTER|DT_SINGLELINE;
       
        if (lpDIS->itemState & ODS_NOACCEL) {
            format  |= DT_HIDEPREFIX;
        }

        int oldBkMode   = ::SetBkMode(lpDIS->hDC, TRANSPARENT);

        ::DrawText(lpDIS->hDC, szMenuString, ::wcslen(szMenuString), &lpDIS->rcItem, format);

        ::SelectObject(lpDIS->hDC, fontOld);
        ::SetBkMode(lpDIS->hDC, oldBkMode);
        ::SetTextColor(lpDIS->hDC, oldRGB);

        return TRUE;
    }

    return FALSE;    

}

void cef_dark_window::HandleNcMouseLeave() 
{
	switch (mNonClientData.mActiveButton)
	{
		case HTCLOSE:
		case HTMAXBUTTON:
		case HTMINBUTTON:
			mNonClientData.mActiveButton = HTNOWHERE;
			mNonClientData.mButtonOver = true;
			mNonClientData.mButtonDown = false;
			UpdateNonClientButtons ();
			break;
	}

	mNonClientData.Reset();
}

BOOL cef_dark_window::HandleNcMouseMove(UINT uHitTest)
{
    if (mNonClientData.mActiveButton != uHitTest) 
    {
        mNonClientData.mActiveButton = uHitTest;

 		mNonClientData.mButtonOver = true;
 		mNonClientData.mButtonDown = false;
	 		
		UpdateNonClientButtons ();
	 
 		switch (uHitTest)
 		{
 		case HTCLOSE:
 		case HTMAXBUTTON:
 		case HTMINBUTTON:
            TrackNonClientMouseEvents();
            return TRUE;
 		}
    }

    return FALSE;
}

BOOL cef_dark_window::HandleNcLeftButtonDown(UINT uHitTest)
{
	mNonClientData.mActiveButton = uHitTest;
	mNonClientData.mButtonOver = true;
	mNonClientData.mButtonDown = true;

    UpdateNonClientButtons ();

	switch (uHitTest)
	{
	case HTCLOSE:
	case HTMAXBUTTON:
	case HTMINBUTTON:
        TrackNonClientMouseEvents();
		return TRUE;

    default:
        return FALSE;
	}
}


BOOL cef_dark_window::HandleNcLeftButtonUp(UINT uHitTest, LPPOINT point)
{
	mNonClientData.mButtonOver = false;
	mNonClientData.mButtonDown = false;

    UpdateNonClientButtons() ;

	switch (mNonClientData.mActiveButton)
	{
	case HTCLOSE:
		SendMessage (WM_SYSCOMMAND, SC_CLOSE, (LPARAM)POINTTOPOINTS(*point));
		mNonClientData.Reset() ;
        TrackNonClientMouseEvents();
		return TRUE;
	case HTMAXBUTTON:
        if (IsZoomed()) 
			SendMessage (WM_SYSCOMMAND, SC_RESTORE, (LPARAM)POINTTOPOINTS(*point));
        else 
		    SendMessage (WM_SYSCOMMAND, SC_MAXIMIZE, (LPARAM)POINTTOPOINTS(*point));
		mNonClientData.Reset() ;
        TrackNonClientMouseEvents();
		return TRUE;
	case HTMINBUTTON:
		if (IsIconic())
			SendMessage (WM_SYSCOMMAND, SC_RESTORE, (LPARAM)POINTTOPOINTS(*point));
		else
			SendMessage (WM_SYSCOMMAND, SC_MINIMIZE, (LPARAM)POINTTOPOINTS(*point));
		mNonClientData.Reset() ;
        TrackNonClientMouseEvents();
		return TRUE;
	}
    
    mNonClientData.Reset();
    return FALSE;
}

LRESULT cef_dark_window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
    case WM_SETTINGCHANGE:
        HandleSettingChange((UINT)wParam, (LPCWSTR)lParam);
        break;
    case WM_NCMOUSELEAVE:
        HandleNcMouseLeave();
        break;
    case WM_NCMOUSEMOVE:
        if (HandleNcMouseMove((UINT)wParam))
            return 0L;
        break;

    case WM_NCLBUTTONDOWN:
        if (HandleNcLeftButtonDown((UINT)wParam))
            return 0L;
        break;
    case WM_NCLBUTTONUP:
        {
            POINT pt;
            POINTSTOPOINT(pt, lParam);
            if (HandleNcLeftButtonUp((UINT)wParam, &pt))
                return 0L;
        }
        break;
    case WM_NCCREATE:
        if (HandleNcCreate())
            return 0L;
        break;  
    case WM_NCPAINT:
        if (HandleNcPaint((HRGN)wParam)) 
            return 0L;
        break;
    case WM_NCDESTROY:
        if (HandleNcDestroy())
            return 0L;
        break;
    case WM_NCHITTEST:
        {
            POINT pt;
            POINTSTOPOINT(pt, lParam);
            return HandleNcHitTest(&pt);
        }            
        break;
    case WM_MEASUREITEM:
        if (HandleMeasureItem((LPMEASUREITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_DRAWITEM:
        if (HandleDrawItem((LPDRAWITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_SETICON:
        mWindowIcon = 0;
        break;
    }

    LRESULT lr = cef_window::WindowProc(message, wParam, lParam);
    
    // post default message processing
    switch (message)
    {
    case WM_SYSCOMMAND:
        HandleSysCommand((UINT)(wParam & 0xFFF0));
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    case WM_MOVE:
    case WM_SIZE:
    case WM_SIZING:
    case WM_EXITSIZEMOVE:
    case WM_NCACTIVATE:
    case WM_ACTIVATE:
    case WM_SETTEXT:
        UpdateNonClientArea();
        break;
    }

    return lr;
}