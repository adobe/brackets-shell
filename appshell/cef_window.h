#pragma once
/*************************************************************************
 *
 * ADOBE CONFIDENTIAL
 * ___________________
 *
 *  Copyright 2013 Adobe Systems Incorporated
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of Adobe Systems Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Adobe Systems Incorporated and its
 * suppliers and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Adobe Systems Incorporated.
 **************************************************************************/
#include <windows.h>
#include <winuser.h>
#include <Shellapi.h>

class cef_window;
class cef_menu;

class cef_window
{
public:
    cef_window(void);
    virtual ~cef_window(void);

    static cef_window* Create(LPCTSTR szClass, LPCTSTR szWindowTitle, DWORD dwStyles, int x, int y, int width, int height, cef_window* parent = NULL, cef_menu* menu = NULL);
    bool SubclassWindow (HWND hWnd);

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT DefaultWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	HMENU GetMenu() 
	{ return ::GetMenu(mWnd); }

	BOOL UpdateWindow()
	{ return ::UpdateWindow(mWnd); }

	BOOL GetWindowPlacement(LPWINDOWPLACEMENT wp)
	{ return ::GetWindowPlacement(mWnd, wp); }

	BOOL SetWindowPlacement(LPWINDOWPLACEMENT wp)
	{ return ::SetWindowPlacement(mWnd, wp); }

	BOOL GetWindowRect(LPRECT r)
	{ return ::GetWindowRect(mWnd, r); }

	BOOL GetClientRect(LPRECT r)
	{ return ::GetClientRect(mWnd, r); }

	HDC BeginPaint(PAINTSTRUCT* ps)
	{ return ::BeginPaint(mWnd, ps); }

	BOOL EndPaint(PAINTSTRUCT* ps) 
	{ return ::EndPaint(mWnd, ps); }

	BOOL DestroyWindow()
	{ return ::DestroyWindow(mWnd); }

    BOOL SetProp(LPCWSTR lpString, HANDLE hData)
    { return ::SetProp(mWnd, lpString, hData); }

    HANDLE GetProp(LPCWSTR lpString)
    { return ::GetProp(mWnd, lpString); }

    HANDLE RemoveProp(LPCWSTR lpString)
    { return ::RemoveProp(mWnd, lpString); }

    LONG GetWindowLongPtr(int nIndex) 
    { return ::GetWindowLongPtr(mWnd, nIndex); }

    LONG SetWindowLongPtr(int nIndex, LONG dwNewLong) 
    { return ::SetWindowLongPtr(mWnd, nIndex, dwNewLong); }

    void DragAcceptFiles(BOOL fAccept)
    { return ::DragAcceptFiles(mWnd, fAccept); }

protected:
    HWND mWnd;
    WNDPROC mSuperWndProc;

	BOOL HandleNonClientDestroy();
	virtual void PostNonClientDestory();
};

