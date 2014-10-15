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
#include "cef_dark_window.h"
#include <dwmapi.h>

// Undocumented Aero Messages
#define WM_UAHDESTROYWINDOW    0x0090
#define WM_UAHDRAWMENU         0x0091
#define WM_UAHDRAWMENUITEM     0x0092
#define WM_UAHINITMENU         0x0093
#define WM_UAHMEASUREMENUITEM  0x0094
#define WM_UAHNCPAINTMENUPOPUP 0x0095

// prototypes for DWM function pointers
typedef HRESULT (STDAPICALLTYPE *PFNDWMEFICA)(HWND hWnd, __in const MARGINS* pMarInset);
typedef BOOL (STDAPICALLTYPE *PFNDWMDWP)(__in HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, __out LRESULT *plResult);
typedef HRESULT (STDAPICALLTYPE *PFNDWMICE)(__out BOOL* pfEnabled);

// manage dynamically loading DesktopWindowManager DLL
class CDwmDLL
{
public:
    CDwmDLL();
    ~CDwmDLL();

    // access to DWM API
    static HRESULT DwmExtendFrameIntoClientArea(HWND hWnd, __in const MARGINS* pMarInset);
    static BOOL DwmDefWindowProc(__in HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, __out LRESULT *plResult);
    static HRESULT DwmIsCompositionEnabled(__out BOOL* pfEnabled);

private:
    HINSTANCE LoadLibrary();
    void FreeLibrary();

private:
    static HINSTANCE mhDwmDll;
    static PFNDWMEFICA pfnDwmExtendFrameIntoClientArea;
    static PFNDWMDWP pfnDwmDefWindowProc;
    static PFNDWMICE pfnDwmIsCompositionEnabled;
};


// Dark window themed window Wrapper
//  Instantiate this class and subclass any HWND to give the window a dark look
class cef_dark_aero_window : public cef_dark_window
{
public:
    // Construction/Destruction - Public Members
    cef_dark_aero_window();
    virtual ~cef_dark_aero_window();

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    virtual BOOL GetRealClientRect(LPRECT r) const;

    bool SubclassWindow(HWND hWnd);

protected:
    // Window Message Handlers
    BOOL HandleActivate();
    BOOL HandleCreate();
    BOOL HandlePaint();
    BOOL HandleNcCalcSize(BOOL calcValidRects, NCCALCSIZE_PARAMS* lpncsp, LRESULT* lr);
    BOOL HandleNcMouseMove(UINT uHitTest, LPPOINT pt);
    BOOL HandleNcLeftButtonDown(UINT uHitTest, LPPOINT pt);
    BOOL HandleGetMinMaxInfo(LPMINMAXINFO mmi);
    BOOL HandleSettingChange(UINT uFlags, LPCWSTR lpszSection);

    void HandleNcMouseLeave();
    int HandleNcHitTest(LPPOINT ptHit);

    void PostHandleNcLeftButtonUp(UINT uHitTest, LPPOINT pt);

    virtual void DrawMenuBar(HDC hdc);
    virtual void HiliteMenuItemAt(LPPOINT pt);

    virtual void UpdateMenuBar();

    virtual void InitDeviceContext(HDC hdc);

    virtual void ComputeMenuBarRect(RECT& rect) const;
    virtual void ComputeWindowCaptionRect(RECT& rect) const;
    virtual void ComputeWindowIconRect(RECT& rect) const;
    virtual void ComputeCloseButtonRect(RECT& rect) const;

    void AdjustRectForAutoHideBars(LPRECT rect) const;

    LRESULT DwpCustomFrameProc(UINT message, WPARAM wParam, LPARAM lParam, bool* pfCallDefWindowProc);

    bool mReady;

private:
    int  mMenuHiliteIndex;
    int  mMenuActiveIndex;
};
