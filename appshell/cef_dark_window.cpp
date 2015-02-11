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

#define OS_WIN
#include "config.h"

//win HiDPI - Macro for loading button resources for scale factors start 
#define BUTTON_RESOURCES(scale)\
    if (mSysCloseButton == NULL) {\
	mSysCloseButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_CLOSE_BUTTON_ ## scale));\
    }\
    if (mSysMaximizeButton == NULL) {\
        mSysMaximizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MAX_BUTTON_ ## scale));\
    }\
    if (mSysMinimizeButton == NULL) {\
        mSysMinimizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MIN_BUTTON_ ## scale));\
    }\
    if (mSysRestoreButton == NULL) {\
        mSysRestoreButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_RESTORE_BUTTON_ ## scale));\
    }\
    if (mHoverSysCloseButton == NULL) {\
        mHoverSysCloseButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_CLOSE_HOVER_BUTTON_ ## scale));\
    }\
    if (mHoverSysRestoreButton == NULL) {\
        mHoverSysRestoreButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_RESTORE_HOVER_BUTTON_ ## scale));\
    }\
    if (mHoverSysMinimizeButton == NULL) {\
        mHoverSysMinimizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MIN_HOVER_BUTTON_ ## scale));\
    }\
    if (mHoverSysMaximizeButton == NULL) {\
        mHoverSysMaximizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MAX_HOVER_BUTTON_ ## scale));\
    }\
    if (mPressedSysCloseButton == NULL) {\
        mPressedSysCloseButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_CLOSE_PRESSED_BUTTON_ ## scale));\
    }\
    if (mPressedSysRestoreButton == NULL) {\
        mPressedSysRestoreButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_RESTORE_PRESSED_BUTTON_ ## scale));\
    }\
    if (mPressedSysMinimizeButton == NULL) {\
        mPressedSysMinimizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MIN_PRESSED_BUTTON_ ## scale));\
    }\
    if (mPressedSysMaximizeButton == NULL) {\
        mPressedSysMaximizeButton = ResourceImage::FromResource(MAKEINTRESOURCE(IDB_MAX_PRESSED_BUTTON_ ## scale));\
    }
//Macro for loading button resources end

// Libraries
#pragma comment(lib, "gdiplus")
#pragma comment(lib, "UxTheme")

// Externals
extern HINSTANCE hInst;

// Module Globals (Not exported)
static ULONG_PTR gdiplusToken = NULL;

// Constants
static const int kMenuPadding = 4;
static const int kWindowFrameZoomFactorCY = 4;
static const int kSystemIconZoomFactorCY = 4;
static const int kSystemIconZoomFactorCX = 2;

// GDI+ Helpers
static void RECT2Rect(Gdiplus::Rect& dest, const RECT& src) {
    dest.X = src.left;
    dest.Y = src.top;   
    dest.Width = ::RectWidth(src);
    dest.Height = ::RectHeight(src);
}

namespace ResourceImage
{
    // Loads an Image from an application's resource section
    //  and returns a GDI+ Image object or NULL on failure
    Gdiplus::Image* FromResource(LPCWSTR lpResourceName)
    {
        // We're only loading PNG images at the present time
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

/*
 * cef_dark_window
 */
cef_dark_window::cef_dark_window() :
    mSysCloseButton(NULL),
    mSysRestoreButton(NULL),
    mSysMinimizeButton(NULL),
    mSysMaximizeButton(NULL),
    mHoverSysCloseButton(NULL),
    mHoverSysRestoreButton(NULL),
    mHoverSysMinimizeButton(NULL),
    mHoverSysMaximizeButton(NULL),
    mPressedSysCloseButton(NULL),
    mPressedSysRestoreButton(NULL),
    mPressedSysMinimizeButton(NULL),
    mPressedSysMaximizeButton(NULL),
    mWindowIcon(NULL),
    mBackgroundActiveBrush(NULL),
    mBackgroundInactiveBrush(NULL),
    mFrameOutlineActivePen(NULL),
    mFrameOutlineInactivePen(NULL),
    mCaptionFont(NULL),
    mMenuFont(NULL),
    mHighlightBrush(NULL),
    mHoverBrush(NULL),
    mIsActive(TRUE)
{
    ::ZeroMemory(&mNcMetrics, sizeof(mNcMetrics));
}

cef_dark_window::~cef_dark_window()
{
}

/*
 * DoFinalCleanup -- Called when the app is shutting down so we can shut down GDI+
 */
void cef_dark_window::DoFinalCleanup()
{
    if (gdiplusToken != NULL) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
    }
}

void cef_dark_window::InitializeWindowForDarkUI() 
{
    // Make sure that the window theme is set
    //  to no theme so we get the right metrics
    ::SetWindowTheme(mWnd, L"", L"");
}

/*
 * InitDrawingResources -- Loads Pens, Brushes and Images used for drawing
 */
void cef_dark_window::InitDrawingResources()
{
    // Fetch Non Client Metric Data 
    //  NOTE: Windows XP chokes if the size include Windows Version 6 members so 
    //          subtract off the size of the new data members
    mNcMetrics.cbSize = sizeof (mNcMetrics) - sizeof (mNcMetrics.iPaddedBorderWidth);
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &mNcMetrics, 0);

    // Startup GDI+
    if (gdiplusToken == NULL) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    LoadSysButtonImages();
  

    // Create Brushes and Pens 
    if (mBackgroundActiveBrush == NULL) {                            
        mBackgroundActiveBrush = ::CreateSolidBrush(CEF_COLOR_BACKGROUND_ACTIVE);
    }
    if (mBackgroundInactiveBrush == NULL) {                            
        mBackgroundInactiveBrush = ::CreateSolidBrush(CEF_COLOR_BACKGROUND_INACTIVE);
    }
    if (mHighlightBrush == NULL) {                            
        mHighlightBrush = ::CreateSolidBrush(CEF_COLOR_MENU_HILITE_BACKGROUND);
    }
    if (mHoverBrush == NULL) {                            
        mHoverBrush = ::CreateSolidBrush(CEF_COLOR_MENU_HOVER_BACKGROUND);
    }
    if (mFrameOutlineActivePen == NULL) {
        mFrameOutlineActivePen = ::CreatePen(PS_SOLID, 1, CEF_COLOR_FRAME_OUTLINE_ACTIVE);
    }
    if (mFrameOutlineInactivePen == NULL) {
        mFrameOutlineInactivePen = ::CreatePen(PS_SOLID, 1, CEF_COLOR_FRAME_OUTLINE_INACTIVE);
    }
}

/*
 * InitMenuFont -- Creates a Font object for drawing menu items
 */
void cef_dark_window::InitMenuFont()
{
    if (mMenuFont == NULL) {
        mMenuFont = ::CreateFontIndirect(&mNcMetrics.lfMenuFont);
    }
}

/*
 * LoadSysButtonImages -- Loads the images used for system buttons (close, min, max, restore) 
 */
void cef_dark_window::LoadSysButtonImages()
{
	UINT scaleFactor = GetDPIScalingX();

    if(scaleFactor>=250)
    {
        BUTTON_RESOURCES(2_5X)
    }
    else if(scaleFactor>=200)
    {
        BUTTON_RESOURCES(2X)
    }
    else if(scaleFactor>=150)
    {
        BUTTON_RESOURCES(1_5X)
    }
    else if(scaleFactor>=125)
    {
        BUTTON_RESOURCES(1_25X)
    }
    else
    {
        BUTTON_RESOURCES(1X)
    }
}

// WM_NCCREATE handler
BOOL cef_dark_window::HandleNcCreate()
{
    InitDrawingResources();
    InitializeWindowForDarkUI();
    return FALSE;
}

bool cef_dark_window::SubclassWindow(HWND hWnd)
{
    InitDrawingResources();
    if (cef_window::SubclassWindow(hWnd)) {
        InitializeWindowForDarkUI();
        return true;
    }
    return false;
}

// WM_NCCREATE handler
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

    delete mPressedSysCloseButton;
    delete mPressedSysRestoreButton;
    delete mPressedSysMinimizeButton;
    delete mPressedSysMaximizeButton;

    ::DeleteObject(mBackgroundActiveBrush);
    ::DeleteObject(mBackgroundInactiveBrush);
    ::DeleteObject(mCaptionFont);
    ::DeleteObject(mMenuFont);
    ::DeleteObject(mHighlightBrush);
    ::DeleteObject(mHoverBrush);
    ::DeleteObject(mFrameOutlineActivePen);
    ::DeleteObject(mFrameOutlineInactivePen);

    return cef_window::HandleNcDestroy();
}

// WM_SETTINGCHANGE handler
BOOL cef_dark_window::HandleSettingChange(UINT uFlags, LPCWSTR lpszSection)
{
    switch (uFlags)
    {
    case SPI_SETICONTITLELOGFONT:
    case SPI_SETHIGHCONTRAST:
    case SPI_SETNONCLIENTMETRICS:
    case SPI_SETFONTSMOOTHING:
    case SPI_SETFONTSMOOTHINGTYPE:
    case SPI_SETFONTSMOOTHINGCONTRAST:
    case SPI_SETFONTSMOOTHINGORIENTATION:
        // Reset our state
        mNonClientData.Reset();

        // Reset Fonts -- they will be recreated when needed
        ::DeleteObject(mCaptionFont);
        mCaptionFont = NULL;

        ::DeleteObject(mMenuFont);
        mMenuFont = NULL;

        // Reload Non Client Metrics
        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &mNcMetrics, 0);
        break;
    }
    return FALSE;
}

// Computes the Rect where the System Icon is drawn in window coordinates
void cef_dark_window::ComputeWindowIconRect(RECT& rect) const
{
    int top = ::GetSystemMetrics (SM_CYFRAME);
    int left = ::GetSystemMetrics (SM_CXFRAME);

    if (IsZoomed()) {
        top += ::kSystemIconZoomFactorCY;
        left += ::kSystemIconZoomFactorCX;
    }
    ::SetRectEmpty(&rect);
    rect.top =  top;
    rect.left = left;
    rect.bottom = rect.top + ::GetSystemMetrics(SM_CYSMICON);
    rect.right = rect.left + ::GetSystemMetrics(SM_CXSMICON);
}

// Computes the Rect where the window caption is drawn in window coordinates
void cef_dark_window::ComputeWindowCaptionRect(RECT& rect) const
{
    RECT wr;
    GetWindowRect(&wr);

    int top = mNcMetrics.iBorderWidth;

    if (IsZoomed()) {
        top = ::kWindowFrameZoomFactorCY;
    }

    rect.top = top;
    rect.bottom = rect.top + mNcMetrics.iCaptionHeight;

    RECT ir;
    ComputeWindowIconRect(ir);

    RECT mr;
    ComputeMinimizeButtonRect(mr);

    rect.left = ir.right + ::GetSystemMetrics (SM_CXFRAME);
    rect.right = mr.left - ::GetSystemMetrics (SM_CXFRAME);
}

// Computes the Rect where the minimize button is drawn in window coordinates
void cef_dark_window::ComputeMinimizeButtonRect(RECT& rect) const
{
    ComputeMaximizeButtonRect(rect);

    rect.left -= (mSysMinimizeButton->GetWidth() + 1);
    rect.right = rect.left + mSysMinimizeButton->GetWidth();
    rect.bottom = rect.top + mSysMinimizeButton->GetHeight();
}

// Computes the Rect where the maximize button is drawn in window coordinates
void cef_dark_window::ComputeMaximizeButtonRect(RECT& rect) const
{
    ComputeCloseButtonRect(rect);

    rect.left -= (mSysMaximizeButton->GetWidth() + 1);
    rect.right = rect.left + mSysMaximizeButton->GetWidth();
    rect.bottom = rect.top + mSysMaximizeButton->GetHeight();
}

// Computes the Rect where the close button is drawn in window coordinates
void cef_dark_window::ComputeCloseButtonRect(RECT& rect) const
{
    int top = mNcMetrics.iBorderWidth;
    int right = mNcMetrics.iBorderWidth;

    if (IsZoomed()) {
        top = ::GetSystemMetrics (SM_CYFRAME);
        right = ::GetSystemMetrics (SM_CXFRAME);
    }

    RECT wr;
    GetWindowRect(&wr);

    rect.left = ::RectWidth(wr) - right - mSysCloseButton->GetWidth();
    rect.top = top;
    rect.right = rect.left + mSysCloseButton->GetWidth();
    rect.bottom = rect.top + mSysCloseButton->GetHeight();
}


void cef_dark_window::ComputeRequiredMenuRect(RECT& rect) const
{
    HMENU menu = GetMenu();
    int items = ::GetMenuItemCount(menu);

    ::SetRectEmpty(&rect);

    for (int i = 0; i < items; i++) {
        RECT itemRect;
        ::SetRectEmpty(&itemRect);
        if (::GetMenuItemRect(mWnd, menu, (UINT)i, &itemRect)) {
            itemRect.bottom += ::kMenuPadding;
            AdjustMenuItemRect(itemRect);

            RECT dest;
            if (::UnionRect(&dest, &rect, &itemRect)) {
                ::CopyRect(&rect, &dest);
            }
        }
    }
}

// Computes the Rect where the menu bar is drawn in window coordinates
void cef_dark_window::ComputeMenuBarRect(RECT& rect) const 
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

// Draw the background for the non-client area
void cef_dark_window::DoDrawFrame(HDC hdc)
{
    RECT rectWindow;
    GetWindowRect(&rectWindow);

    RECT rectFrame;
    ::SetRectEmpty(&rectFrame);

    rectFrame.bottom = ::RectHeight(rectWindow);
    rectFrame.right = ::RectWidth(rectWindow);

    // Paint the entire thing with the background brush
    ::FillRect(hdc, &rectFrame, (mIsActive) ? mBackgroundActiveBrush : mBackgroundInactiveBrush);

    HGDIOBJ oldPen = (mIsActive) ? ::SelectObject(hdc, mFrameOutlineActivePen) : ::SelectObject(hdc, mFrameOutlineInactivePen);
    HGDIOBJ oldbRush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));

    // Now draw a PX tuxedo border around the edge
    ::Rectangle(hdc, 0, 0, rectFrame.right, rectFrame.bottom);

    ::SelectObject(hdc, oldPen);
    ::SelectObject(hdc, oldbRush);
}

// Draw the System Icon
void cef_dark_window::DoDrawSystemMenuIcon(HDC hdc)
{
    if (mWindowIcon == 0) {
        // We haven't cached the icon yet so figure out 
        //  which one we need.

        // First try to load the small icon 
        mWindowIcon = (HICON)SendMessage(WM_GETICON, ICON_SMALL, 0);

        // Otherwise try to use the big icon
        if (!mWindowIcon) 
            mWindowIcon = (HICON)SendMessage(WM_GETICON, ICON_BIG, 0);

        // If that doesn't work check the window class for an icon

        // Start with the small
        if (!mWindowIcon)
            mWindowIcon = reinterpret_cast<HICON>(GetClassLongPtr(GCLP_HICONSM));
        
        // Then try to load the big icon
        if (!mWindowIcon)
            mWindowIcon = reinterpret_cast<HICON>(GetClassLongPtr(GCLP_HICON));

        // Otherwise we need an icon, so just use the standard Windows default 
        //  application Icon which may very between versions 
        if (!mWindowIcon) 
            mWindowIcon = ::LoadIcon(NULL, IDI_APPLICATION);
    }

    RECT rectIcon;
    ComputeWindowIconRect(rectIcon);
    ::DrawIconEx(hdc, rectIcon.left, rectIcon.top, mWindowIcon, ::RectWidth(rectIcon), ::RectHeight(rectIcon), 0, NULL, DI_NORMAL);
}

// Draw the Caption Bar
void cef_dark_window::DoDrawTitlebarText(HDC hdc)
{
    if (mCaptionFont == 0) {
        mCaptionFont = ::CreateFontIndirect(&mNcMetrics.lfCaptionFont);
    }

    RECT textRect;
    ComputeWindowCaptionRect(textRect);

    HGDIOBJ hPreviousFont = ::SelectObject(hdc, mCaptionFont);        
    int oldBkMode = ::SetBkMode(hdc, TRANSPARENT);
    COLORREF oldRGB = ::SetTextColor(hdc, CEF_COLOR_NORMALTEXT);

    // Setup the rect to use to calculate the position
    RECT windowRect;
    GetWindowRect(&windowRect);
    ScreenToNonClient(&windowRect);

    // Get the title and the length
    int textLength = GetWindowTextLength() + 1;
    LPWSTR szCaption = new wchar_t [textLength + 1];
    ::ZeroMemory(szCaption, textLength + 1);
    int cchCaption = GetWindowText(szCaption, textLength);

    // Figure out how much space we need to draw ethe whole thing
    RECT rectTemp;
    ::SetRectEmpty(&rectTemp);
    ::DrawText(hdc, szCaption, ::wcslen(szCaption), &rectTemp, DT_SINGLELINE|DT_CALCRECT|DT_NOPREFIX);

    // Can it be centered within the window?
    if (((::RectWidth(windowRect) / 2) + (::RectWidth(rectTemp) / 2) + 1) < textRect.right) {
        windowRect.bottom = textRect.bottom;
        windowRect.top += textRect.top;
        ::DrawText(hdc, szCaption, cchCaption, &windowRect, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    } else {
        // Can't be centered so LEFT justify and add ellipsis when truncated
        ::DrawText(hdc, szCaption, cchCaption, &textRect, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    }
 
    delete []szCaption;
    ::SetTextColor(hdc, oldRGB);
    ::SetBkMode(hdc, oldBkMode);
    ::SelectObject(hdc, hPreviousFont);
}

// Draw the System Icons (close/min/max/restore)
void cef_dark_window::DoDrawSystemIcons(HDC hdc)
{
    Gdiplus::Image* CloseButton = mSysCloseButton;
    Gdiplus::Image* RestoreButton = mSysRestoreButton;
    Gdiplus::Image* MinimizeButton = mSysMinimizeButton;
    Gdiplus::Image* MaximizeButton = mSysMaximizeButton;

    if (mNonClientData.mButtonOver) {
        // If the mouse is over or down on a button then 
        //  we need to pick the correct image for that button's state
        switch (mNonClientData.mActiveButton)
        {
        case HTCLOSE:                
            CloseButton = (mNonClientData.mButtonDown) ? mPressedSysCloseButton : mHoverSysCloseButton;
            break;
 
        case HTMAXBUTTON:
            RestoreButton = (mNonClientData.mButtonDown) ? mPressedSysRestoreButton : mHoverSysRestoreButton;
            MaximizeButton = (mNonClientData.mButtonDown) ? mPressedSysMaximizeButton : mHoverSysMaximizeButton;
            break;
 
        case HTMINBUTTON:
            MinimizeButton = (mNonClientData.mButtonDown) ? mPressedSysMinimizeButton : mHoverSysMinimizeButton;
            break ;
        }
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

// Enforce that the menu has our background color otherwise it 
//  uses the system default color for the menu bar which looks funny
void cef_dark_window::EnforceMenuBackground()
{
    MENUBARINFO mbi = {0};
    mbi.cbSize = sizeof(mbi);
    
    GetMenuBarInfo(OBJID_MENU, 0, &mbi);

    MENUINFO mi = {0};
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_BACKGROUND;

    ::GetMenuInfo(mbi.hMenu, &mi);
    
    if (mIsActive) {
        if (mi.hbrBack != mBackgroundActiveBrush) {
            mi.hbrBack = mBackgroundActiveBrush;
            ::SetMenuInfo(mbi.hMenu, &mi);
        }
    } else {
        if (mi.hbrBack != mBackgroundInactiveBrush) {
            mi.hbrBack = mBackgroundInactiveBrush;
            ::SetMenuInfo(mbi.hMenu, &mi);
        }
    }
}

// Setup the device context for drawing
void cef_dark_window::InitDeviceContext(HDC hdc)
{
    RECT rectClipClient;
    SetRectEmpty(&rectClipClient);
    ComputeLogicalClientRect(rectClipClient);

    // exclude the client area to reduce flicker
    ::ExcludeClipRect(hdc, rectClipClient.left, rectClipClient.top, rectClipClient.right, rectClipClient.bottom);
}

void cef_dark_window::AdjustMenuItemRect(RECT &itemRect) const
{
    if (CanUseAeroGlass() && IsZoomed()) {
        ScreenToClient(&itemRect);
    } else {
        ScreenToNonClient(&itemRect);
    }
}

// This Really just Draws the menu bar items since it assumes 
//  that the background has already been drawn using DoDrawFrame
void cef_dark_window::DoDrawMenuBar(HDC hdc)
{
    HMENU menu = GetMenu();
    int items = ::GetMenuItemCount(menu);

    RECT menuBarRect;
    ComputeMenuBarRect(menuBarRect);

    for (int i = 0; i < items; i++) {
        // Determine the menu item state and ID
        MENUITEMINFO mmi = {0};
        mmi.cbSize = sizeof (mmi);
        mmi.fMask = MIIM_STATE|MIIM_ID;
        ::GetMenuItemInfo (menu, i, TRUE, &mmi);

        RECT itemRect;
        ::SetRectEmpty(&itemRect);

        if (::GetMenuItemRect(mWnd, menu, (UINT)i, &itemRect)) {
            AdjustMenuItemRect(itemRect);
            
            POINT ptTopLeftItem = {itemRect.left, itemRect.top}; 

            if (CanUseAeroGlass() && !::PtInRect(&menuBarRect, ptTopLeftItem)) {
                // Check to make sure it's actually in the 
                //  the correct location (fixes aero drawing issue)
                int itemHeight = ::RectHeight(itemRect);
                itemRect.top = menuBarRect.top;
                itemRect.bottom = itemRect.top + itemHeight;
            }

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
}

// Paints the entire non-client area.  
//  Creates an in-memory DC to draw on to reduce flicker
void cef_dark_window::DoPaintNonClientArea(HDC hdc)
{
    EnforceMenuBackground();
    cef_buffered_dc dc(this, hdc);

    InitDeviceContext(dc);
    InitDeviceContext(dc.GetWindowDC());

    DoDrawFrame(dc);
    DoDrawSystemMenuIcon(dc);
    DoDrawTitlebarText(dc);
    DoDrawSystemIcons(dc);
    DoDrawMenuBar(dc);
}

// Special case for derived classes to implement
//  in order to repaint the client area when we've
//  turned off redraw and turn it back on.
void cef_dark_window::DoRepaintClientArea()
{
}

// Force Drawing the non-client area.
//  Normally WM_NCPAINT is used but there are times when you
//  need to force drawing the entire non-client area when
//  legacy windows message handlers start drawing non-client
//  artifacts over top of us
void cef_dark_window::UpdateNonClientArea()
{
    HDC hdc;
    if (CanUseAeroGlass()) {
        hdc = GetDC();
    } else {
        hdc = GetWindowDC();
    }

    DoPaintNonClientArea(hdc);
    ReleaseDC(hdc);
}

// WM_NCPAINT handler
BOOL cef_dark_window::HandleNcPaint(HRGN hUpdateRegion)
{
    HDC hdc = GetDCEx(hUpdateRegion, DCX_WINDOW|DCX_INTERSECTRGN|DCX_USESTYLE);
    DoPaintNonClientArea(hdc);
    ReleaseDC(hdc);

    return TRUE;
}


// WM_NCHITTEST handler
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

    return HTCAPTION;
}

// Like UpdateNonClientArea, but sets up a clipping region to just update the system buttons
void cef_dark_window::UpdateNonClientButtons () 
{
    // create a simple clipping region
    //  that only includes the system buttons (min/max/restore/close)
    HDC hdc;

    if (CanUseAeroGlass()) {
        hdc = GetDC();
    } else {
        hdc = GetWindowDC();
    }

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
        DoPaintNonClientArea(hdc);
    }

    ::DeleteObject(hrgnUpdate);
    ReleaseDC(hdc);
}

// WM_MEASUREITEM handler
// Since we have owner drawn menus, we need to provide information about 
//  the menus back to Windows so it can layout the menubar appropriately.
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

        lpMIS->itemHeight = ::RectHeight(rectTemp);
        lpMIS->itemWidth = ::RectWidth(rectTemp);
 
        ::SelectObject(dc, fontOld);            
        ReleaseDC(dc);
        return TRUE;
    }

    return FALSE;
}

// WM_DRAWITEM handler
// Handles drawing the menu bar items with a dark background
BOOL cef_dark_window::HandleDrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    static wchar_t szMenuString[256] = L"";

    if (lpDIS->CtlType == ODT_MENU) {

        InitMenuFont();

        HGDIOBJ fontOld = ::SelectObject(lpDIS->hDC, mMenuFont);            
        COLORREF rgbMenuText =  CEF_COLOR_NORMALTEXT;
        HMENU hMenu = (HMENU)lpDIS->hwndItem;

        ::GetMenuString(hMenu, lpDIS->itemID, szMenuString, _countof(szMenuString), MF_BYCOMMAND);
        
        RECT rectBG;
        // Add a little more room to the top fixes highlight issue
        ::CopyRect(&rectBG, &lpDIS->rcItem);
        rectBG.top -= 2;

        if (lpDIS->itemState & ODS_SELECTED || lpDIS->itemState & ODS_HOTLIGHT) {
            ::FillRect(lpDIS->hDC, &rectBG, mHoverBrush);
        } else {
            ::FillRect(lpDIS->hDC, &rectBG, (mIsActive) ? mBackgroundActiveBrush : mBackgroundInactiveBrush);
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

/* 
 * The mouse handlers below will track the state of 
 *  the mouse and the system buttons so the drawing code
 *  can show the correct state and fire the correct command.
 * 
 * NonClientData.mActiveButton  HitTest for the button the mouse is over/pushed
 * NonClientData.mButtonDown    Indicates whether the mouse button is down or not
 *                                  The button down state is only an indicator that the
 *                                  action performed by the mouse *could* result in 
 *                                  execution of the SysCommand. 
 * NonClientData.mButtonOver    Indicates whether the mouse is over mActiveButton
 *                                  Once the mouse button has been pressed -- mActiveButton
 *                                  will not update on mouse moves until the mouse button
 *                                  is released.  mButtonOver, however, will change during
 *                                  mouse moves to indicate that the mouse is indeed over
 *                                  the button that was pressed.  
 *                              This prevents misfires when the button is pressed and 
 *                                  held and moved to another button and released.
 */

// WM_NCMOUSELEAVE handler 
void cef_dark_window::HandleNcMouseLeave() 
{
    // When the mouse leaves the non-client area
    //  we just need to reset our button state and 
    //  redraw the system buttons
    mNonClientData.Reset();
    UpdateNonClientButtons();
}

// WM_NCMOUSEMOVE handler 
BOOL cef_dark_window::HandleNcMouseMove(UINT uHitTest)
{
    bool needUpdate = false;

    if (mNonClientData.mActiveButton != uHitTest) 
    {
        needUpdate = true;
        if (mNonClientData.mButtonDown) {
            // If the mouse button was down, we just want 
            //  to indicate that we're no longer over the system button
            mNonClientData.mButtonOver = false;
        } else {
            switch (uHitTest)
            {
            case HTCLOSE:
            case HTMAXBUTTON:
            case HTMINBUTTON:
                // Otherwise, we're just mousing over stuff 
                //  so we need to show the visual feedback
                mNonClientData.mButtonOver = true;
                mNonClientData.mActiveButton = uHitTest;
                TrackNonClientMouseEvents();
                break;
            default:
                // User isn't hovering over anything that 
                //  we care about so just reset and redraw
                mNonClientData.Reset();
                UpdateNonClientButtons();
                // let the DefaultWindowProc handle this 
                return FALSE;
            }
        }
    } else {
        // Check to see if the user moved the mouse back over
        //  the button that they originally click and held and
        //  redraw the button in the correct state if they did
        if (mNonClientData.mButtonDown && !mNonClientData.mButtonOver) {
            mNonClientData.mButtonOver = true;
            needUpdate = true;
        }
    }

    if (needUpdate) {
        UpdateNonClientButtons();
    }
    return TRUE;
}


// WM_NCLBUTTONDOWN handler 
BOOL cef_dark_window::HandleNcLeftButtonDown(UINT uHitTest)
{
    switch (uHitTest)
    {
    case HTCLOSE:
    case HTMAXBUTTON:
    case HTMINBUTTON:
        // we only care about these guys
        mNonClientData.mActiveButton = uHitTest;
        mNonClientData.mButtonOver = true;
        mNonClientData.mButtonDown = true;
        UpdateNonClientButtons();
        TrackNonClientMouseEvents();
        return TRUE;
    default:
        mNonClientData.Reset();
        UpdateNonClientButtons();
        return FALSE;
    }
}

// WM_NCLBUTTONUP handler 
BOOL cef_dark_window::HandleNcLeftButtonUp(UINT uHitTest, LPPOINT point)
{
    NonClientButtonStateData oldData(mNonClientData);

    mNonClientData.Reset();

    UpdateNonClientButtons();

    switch (oldData.mActiveButton) 
    {
    case HTCLOSE:
        if (oldData.mButtonOver) {
            SendMessage (WM_SYSCOMMAND, SC_CLOSE, (LPARAM)POINTTOPOINTS(*point));
            TrackNonClientMouseEvents(false);
        }
        return TRUE;
    case HTMAXBUTTON:
        if (oldData.mButtonOver) {
            if (IsZoomed()) 
                SendMessage (WM_SYSCOMMAND, SC_RESTORE, (LPARAM)POINTTOPOINTS(*point));
            else 
                SendMessage (WM_SYSCOMMAND, SC_MAXIMIZE, (LPARAM)POINTTOPOINTS(*point));
            TrackNonClientMouseEvents(false);
        }
        return TRUE;
    case HTMINBUTTON:
        if (oldData.mButtonOver) {
            SendMessage (WM_SYSCOMMAND, SC_MINIMIZE, (LPARAM)POINTTOPOINTS(*point));
            TrackNonClientMouseEvents(false);
        }
        return TRUE;
    }
    
    return FALSE;
}

void cef_dark_window::DoPaintClientArea(HDC hdc)
{
    
}


// WindowProc handles dispatching of messages and routing back to the base class or to Windows
LRESULT cef_dark_window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lr = 0;

    switch (message) 
    {
    case WM_SETTINGCHANGE:
        // NOTE: We want anyone else interested in this message
        //          to be notified of a setting change even if we handle 
        //          the message.  Otherwise the default implementation 
        //          may be in the wrong state
        HandleSettingChange((UINT)wParam, (LPCWSTR)lParam);
        break;
    case WM_NCMOUSELEAVE:
        // NOTE: We want anyone else interested in this message
        //          to be notified. Otherwise the default implementation 
        //          may be in the wrong state
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
    case WM_SETTEXT:
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
        // Turn off redraw because the 
        //  DefaultWindowProc will paint over top of 
        //  our frame and cause the title bar to flicker 
        if (!IsIconic()){
            SetRedraw(FALSE); 
        }
        break;
    }
    lr = cef_window::WindowProc(message, wParam, lParam);
    // post default message processing
    switch (message)
    {
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    case WM_MOVE:
    case WM_SIZE:
    case WM_SIZING:
    case WM_EXITSIZEMOVE:
        UpdateNonClientArea();
        break;
    }

    // special case -- since we turned off redraw for these 
    //  messages to reduce flicker, we need to turn redraw 
    //  on now and force updating the non-client area
    switch (message) 
    {
    case WM_SETTEXT:
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
        if (!IsIconic()){
            SetRedraw(TRUE);
            UpdateNonClientArea();
            if (message != WM_SETTEXT) {
                DoRepaintClientArea();
            }
        }
        break;
    case WM_ACTIVATEAPP:
        mIsActive = (BOOL)wParam;
        UpdateNonClientArea();
        break;
    }
    return lr;
}
