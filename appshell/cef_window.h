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
#include <windows.h>
#include <winuser.h>
#include <Shellapi.h>

class cef_window;
class cef_menu;

// RECT helpers
static __inline void RectSwapLeftRight(RECT &r) 
{
    LONG temp = r.left;
    r.left = r.right;
    r.right = temp;
}

static __inline void NormalizeRect(RECT& r) 
{
	int nTemp;
	if (r.left > r.right)
	{
		nTemp = r.left;
		r.left = r.right;
		r.right = nTemp;
	}
	if (r.top > r.bottom)
	{
		nTemp = r.top;
		r.top = r.bottom;
		r.bottom = nTemp;
	}
}

static __inline int RectWidth(const RECT &rIn) 
{ 
	RECT r;
	::CopyRect(&r, &rIn);
	::NormalizeRect(r);
	return r.right - r.left; 
}

static __inline int RectHeight(const RECT &rIn) 
{ 
	RECT r;
	::CopyRect(&r, &rIn);
	::NormalizeRect(r);
	return r.bottom - r.top; 
}


// Undocumented Flags for GetDCEx()
#ifndef DCX_USESTYLE
#define DCX_USESTYLE 0x00010000
#endif


// cef_window is a basic HWND wrapper
//  that can be used to wrap any HWND 
class cef_window
{
public:
    // Construciton/Destruction - Public Members
    cef_window(void);
    virtual ~cef_window(void);

    // Initialization
    BOOL Create(LPCTSTR szClass, LPCTSTR szWindowTitle, DWORD dwStyles, int x, int y, int width, int height, cef_window* parent = NULL, cef_menu* menu = NULL);
    bool SubclassWindow (HWND hWnd);

    // Window Message Handling
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT DefaultWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    // Message Sending
    LRESULT SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    { return ::SendMessage(mWnd, uMsg, wParam, lParam); }

    BOOL PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    { return ::PostMessage(mWnd, uMsg, wParam, lParam); }

    // Logistics
    HWND GetSafeWnd() const
    { return (this != NULL) ? mWnd : NULL; }

    // Various Win API helpers
    HMENU GetMenu() const
    { return ::GetMenu(mWnd); }

    BOOL UpdateWindow()
    { return ::UpdateWindow(mWnd); }

    BOOL GetWindowPlacement(LPWINDOWPLACEMENT wp)
    { return ::GetWindowPlacement(mWnd, wp); }

    BOOL SetWindowPlacement(LPWINDOWPLACEMENT wp)
    { return ::SetWindowPlacement(mWnd, wp); }

    BOOL GetWindowRect(LPRECT r) const
    { return ::GetWindowRect(mWnd, r); }

    BOOL GetClientRect(LPRECT r) const
    { return ::GetClientRect(mWnd, r); }
    
    void NonClientToScreen(LPRECT r) const;
    void ScreenToNonClient(LPRECT r) const;

    void ClientToScreen(LPRECT r) const;
    void ScreenToClient(LPRECT lpRect) const;

    HDC BeginPaint(PAINTSTRUCT* ps)
    { return ::BeginPaint(mWnd, ps); }

    BOOL EndPaint(PAINTSTRUCT* ps) 
    { return ::EndPaint(mWnd, ps); }

    BOOL DestroyWindow()
    { return ::DestroyWindow(mWnd); }

    BOOL SetProp(LPCWSTR lpString, HANDLE hData)
    { return ::SetProp(mWnd, lpString, hData); }

    HANDLE GetProp(LPCWSTR lpString) const
    { return ::GetProp(mWnd, lpString); }

    HANDLE RemoveProp(LPCWSTR lpString)
    { return ::RemoveProp(mWnd, lpString); }

    LONG GetWindowLongPtr(int nIndex)  const
    { return ::GetWindowLongPtr(mWnd, nIndex); }

    LONG SetWindowLongPtr(int nIndex, LONG dwNewLong) 
    { return ::SetWindowLongPtr(mWnd, nIndex, dwNewLong); }

    LONG GetClassLongPtr(int nIndex) const
    { return ::GetClassLongPtr(mWnd, nIndex); }

    BOOL GetWindowInfo (PWINDOWINFO pwi) const
    { return ::GetWindowInfo (mWnd, pwi); }

    void DragAcceptFiles(BOOL fAccept)
    { return ::DragAcceptFiles(mWnd, fAccept); }

    BOOL ShowWindow(int nCmdShow)
    { return ::ShowWindow(mWnd, nCmdShow); }

    HDC GetDCEx(HRGN hrgnClip, DWORD dwFlags)
    { return ::GetDCEx(mWnd, hrgnClip, dwFlags); }

    HDC GetWindowDC()
    { return ::GetWindowDC(mWnd); }

    HDC GetDC() const
    { return ::GetDC(mWnd); }

    int ReleaseDC(HDC dc) const
    { return ::ReleaseDC(mWnd, dc); }

    BOOL SetWindowPos(cef_window* insertAfter, int x, int y, int cx, int cy, UINT uFlags) 
    { return ::SetWindowPos(mWnd, insertAfter->GetSafeWnd(), x, y, cx, cy, uFlags); }

    int GetWindowText(LPWSTR lpString, int nMaxCount)  const
    { return ::GetWindowTextW(mWnd, lpString, nMaxCount); }

    int GetWindowTextLength() const
    { return ::GetWindowTextLengthW(mWnd); }

    BOOL InvalidateRect(LPRECT lpRect, BOOL bErase = FALSE)
    { return ::InvalidateRect(mWnd, lpRect, bErase); }

    BOOL IsZoomed() const
    { return ::IsZoomed(mWnd); }

    BOOL IsIconic() const
    { return ::IsIconic(mWnd); }

    BOOL IsWindowVisible() const 
    { return ::IsWindowVisible(mWnd); }

    void SetStyle(DWORD dwStyle) 
    { SetWindowLong(GWL_STYLE, dwStyle); }

    DWORD GetStyle() const
    { return GetWindowLong(GWL_STYLE); }

    void RemoveStyle(DWORD dwStyle) 
    { SetStyle(GetStyle() & ~dwStyle); }
    
    void AddStyle(DWORD dwStyle) 
    { SetStyle(GetStyle() & dwStyle); }

    void SetExStyle(DWORD dwExStyle) 
    { SetWindowLong(GWL_EXSTYLE, dwExStyle); }

    DWORD GetExStyle() const
    { return GetWindowLong(GWL_EXSTYLE); }

    void RemoveExStyle(DWORD dwExStyle)
    { SetExStyle(GetExStyle() & ~dwExStyle); }

    void AddStyleEx(DWORD dwExStyle) 
    { SetExStyle(GetExStyle() & dwExStyle); }

    BOOL GetMenuBarInfo(LONG idObject, LONG idItem, LPMENUBARINFO pmbi) 
    { return ::GetMenuBarInfo(mWnd, idObject, idItem, pmbi); }

    void SetRedraw(BOOL bRedraw)
    { DefaultWindowProc(WM_SETREDRAW, (WPARAM)bRedraw, 0); }

    HWND GetWindow(UINT uCmd) 
    { return ::GetWindow(mWnd, uCmd); }

	UINT GetDPIScalingX() const;

protected:
    // Attributes - Protected Members
    HWND mWnd;
    WNDPROC mSuperWndProc;

    // Implementation
    BOOL HandleNcDestroy();
    BOOL HandleCreate();
    virtual void PostNcDestroy();

    void ComputeLogicalClientRect(RECT& rectClient) const;
    void ComputeLogicalWindowRect (RECT& rectWindow) const;

    BOOL TrackNonClientMouseEvents(bool track = true);
};
