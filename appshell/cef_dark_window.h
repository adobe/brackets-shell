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
#include "cef_window.h"
#include "cef_buffered_dc.h"

// Placeholder 
namespace Gdiplus
{
    class Image;
};

// Color Constants
#define CEF_COLOR_BACKGROUND_ACTIVE         RGB(60, 63, 65)
#define CEF_COLOR_BACKGROUND_INACTIVE       RGB(68, 71, 73)
#define CEF_COLOR_FRAME_OUTLINE_ACTIVE      RGB(42, 44, 45)
#define CEF_COLOR_FRAME_OUTLINE_INACTIVE    RGB(60, 63, 65)
#define CEF_COLOR_NORMALTEXT                RGB(215, 216, 217)
#define CEF_COLOR_MENU_HILITE_BACKGROUND    RGB(247, 247, 247)
#define CEF_COLOR_MENU_HOVER_BACKGROUND     RGB(45, 46, 48)
#define CEF_COLOR_MENU_SELECTED_TEXT        RGB(30, 30, 30)
#define CEF_COLOR_MENU_DISABLED_TEXT        RGB(130, 130, 130)

// Dark window themed window Wrapper
//  Instantiate this class and subclass any HWND to give the window a dark look
class cef_dark_window : public cef_window
{
    // Non-client button state data
    class NonClientButtonStateData
    {
    public:
        NonClientButtonStateData() 
        {
            Reset() ;
        }

        NonClientButtonStateData(const NonClientButtonStateData &other) :
            mActiveButton(other.mActiveButton),
            mButtonDown(other.mButtonDown),
            mButtonOver(other.mButtonOver)
        {
        
        }

        void Reset ( void ) 
        {
            mActiveButton = HTNOWHERE;
            mButtonDown = false;
            mButtonOver = false;
        }

        UINT mActiveButton;
        bool mButtonDown;
        bool mButtonOver;
    };

public:
    // Construction/Destruction - Public Members
    cef_dark_window();
    virtual ~cef_dark_window();

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    bool SubclassWindow(HWND hWnd);

    // must be implemented in derived class
    virtual BOOL CanUseAeroGlass() const
    { return false; }

protected:
    // Window Message Handlers
    BOOL HandleNcCreate();
    BOOL HandleNcDestroy();
    BOOL HandleNcPaint(HRGN hUpdateRegion);
    BOOL HandleNcMouseMove(UINT uHitTest);
    BOOL HandleNcLeftButtonUp(UINT uHitTest, LPPOINT point);
    BOOL HandleNcLeftButtonDown(UINT uHitTest);
    BOOL HandleMeasureItem(LPMEASUREITEMSTRUCT lpMIS);
    BOOL HandleDrawItem(LPDRAWITEMSTRUCT lpDIS);
    BOOL HandleSettingChange(UINT uFlags, LPCWSTR lpszSection);

    int HandleNcHitTest(LPPOINT ptHit);
    void HandleNcMouseLeave();

    // Drawing Helpers
    virtual void UpdateNonClientArea();
    virtual void UpdateNonClientButtons();

    // Drawing Initializer
    virtual void InitDeviceContext(HDC hdc);

    // Drawing Methods
    virtual void DoPaintClientArea(HDC hdc);
    virtual void DoPaintNonClientArea(HDC hdc);
    virtual void DoDrawFrame(HDC hdc);
    virtual void DoDrawSystemMenuIcon(HDC hdc);
    virtual void DoDrawTitlebarText(HDC hdc);
    virtual void DoDrawSystemIcons(HDC hdc);
    virtual void DoDrawMenuBar(HDC hdc);

    // Special Drawing Helpers
    virtual void DoRepaintClientArea();

    // Rect Computers
    virtual void ComputeWindowIconRect(RECT& rect) const;
    virtual void ComputeWindowCaptionRect(RECT& rect) const;
    virtual void ComputeMinimizeButtonRect(RECT& rect) const;
    virtual void ComputeMaximizeButtonRect(RECT& rect) const;
    virtual void ComputeCloseButtonRect(RECT& rect) const;
    virtual void ComputeMenuBarRect(RECT& rect) const;
    virtual void ComputeRequiredMenuRect(RECT& rect) const;
  
    
    // Drawing Initializers
    void InitDrawingResources();
    void LoadSysButtonImages();

    // Menu Initializers 
    void InitMenuFont();
    void EnforceMenuBackground();

    // Basic Initialization
    void InitializeWindowForDarkUI();   

    // Teardown
    void DoFinalCleanup();

    // Images, Fonts and Brushes used for Drawing
    Gdiplus::Image*              mSysCloseButton;
    Gdiplus::Image*              mSysRestoreButton;
    Gdiplus::Image*              mSysMinimizeButton;
    Gdiplus::Image*              mSysMaximizeButton;
    Gdiplus::Image*              mHoverSysCloseButton;
    Gdiplus::Image*              mHoverSysRestoreButton;
    Gdiplus::Image*              mHoverSysMinimizeButton;
    Gdiplus::Image*              mHoverSysMaximizeButton;
    Gdiplus::Image*              mPressedSysCloseButton;
    Gdiplus::Image*              mPressedSysRestoreButton;
    Gdiplus::Image*              mPressedSysMinimizeButton;
    Gdiplus::Image*              mPressedSysMaximizeButton;

    BOOL                         mIsActive;

    HFONT                        mCaptionFont;
    HBRUSH                       mBackgroundActiveBrush;
    HBRUSH                       mBackgroundInactiveBrush;
    HICON                        mWindowIcon;

    HFONT                        mMenuFont;
    HBRUSH                       mHighlightBrush;
    HBRUSH                       mHoverBrush;
    HPEN                         mFrameOutlineActivePen; 
    HPEN                         mFrameOutlineInactivePen; 

    // Metrics and State Data
    NONCLIENTMETRICS             mNcMetrics;
    NonClientButtonStateData     mNonClientData;
};
