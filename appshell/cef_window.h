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
	{ return ::GetMenu(m_hWnd); }

	BOOL UpdateWindow()
	{ return ::UpdateWindow(m_hWnd); }

	BOOL GetWindowPlacement(LPWINDOWPLACEMENT wp)
	{ return ::GetWindowPlacement(m_hWnd, wp); }

	BOOL SetWindowPlacement(LPWINDOWPLACEMENT wp)
	{ return ::SetWindowPlacement(m_hWnd, wp); }

	BOOL GetWindowRect(LPRECT r)
	{ return ::GetWindowRect(m_hWnd, r); }

	BOOL GetClientRect(LPRECT r)
	{ return ::GetClientRect(m_hWnd, r); }

	HDC BeginPaint(PAINTSTRUCT* ps)
	{ return ::BeginPaint(m_hWnd, ps); }

	BOOL EndPaint(PAINTSTRUCT* ps) 
	{ return ::EndPaint(m_hWnd, ps); }

	BOOL DestroyWindow()
	{ return ::DestroyWindow(m_hWnd); }

    BOOL SetProp(LPCWSTR lpString, HANDLE hData)
    { return ::SetProp(m_hWnd, lpString, hData); }

    HANDLE SetProp(LPCWSTR lpString)
    { return ::GetProp(m_hWnd, lpString); }

    HANDLE RemoveProp(LPCWSTR lpString)
    { return ::RemoveProp(m_hWnd, lpString); }

protected:
    HWND m_hWnd;
    WNDPROC m_superWndProc;

	BOOL HandleNonClientDestroy();
	virtual void PostNonClientDestory();
};

