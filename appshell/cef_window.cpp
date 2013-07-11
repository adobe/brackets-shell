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
#include "cef_window.h"

extern HINSTANCE gInstance;

struct HookData {
    HookData() 
    {
        this->Reset();
    }
    void Reset()
    {
        m_hOldHook = NULL;
        m_window = NULL;        
    }

    HHOOK m_hOldHook;
    cef_window* m_window;
} g_HookData;




cef_window::cef_window(void) :
    m_hWnd(NULL),
    m_superWndProc(NULL)
{
}


cef_window::~cef_window(void)
{
}

static LRESULT CALLBACK _HookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code != HCBT_CREATEWND)
        return CallNextHookEx(g_HookData.m_hOldHook, code, wParam, lParam);
    
    LPCREATESTRUCT lpcs = ((LPCBT_CREATEWND)lParam)->lpcs;

    if (lpcs->lpCreateParams == (LPVOID)g_HookData.m_window) 
    {
        HWND hWnd = (HWND)wParam;
        g_HookData.m_window->SubclassWindow(hWnd);
        g_HookData.Reset();
    }

    return CallNextHookEx(g_HookData.m_hOldHook, code, wParam, lParam);
}

static void _HookWindowCreate(cef_window* window)
{
    if (g_HookData.m_hOldHook || g_HookData.m_window) 
        return;

    g_HookData.m_hOldHook = ::SetWindowsHookEx(WH_CBT, _HookProc, NULL, ::GetCurrentThreadId());
    g_HookData.m_window = window;
}


static void _UnHookWindowCreate()
{
    g_HookData.Reset();
}

static LRESULT CALLBACK _WindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  cef_window* window = (cef_window*)::GetProp(hWnd, L"CefClientWindowPtr");
  if (window) 
  {
      return window->WindowProc(message, wParam, lParam);
  } 
  else 
  {
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

bool cef_window::SubclassWindow(HWND hWnd) 
{
    if (::GetProp(hWnd, L"CefClientWindowPtr") != NULL) 
    {
        return false;
    }
	m_hWnd = hWnd;
    m_superWndProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&_WindowProc);
    SetProp(L"CefClientWindowPtr", (HANDLE)this);
    return true;
}

cef_window* cef_window::Create(LPCTSTR szClassname, LPCTSTR szWindowTitle, DWORD dwStyles, int x, int y, int width, int height, cef_window* parent/*=NULL*/, cef_menu* menu/*=NULL*/)
{
    HWND hWndParent = parent ? parent->m_hWnd : NULL;
    HMENU hMenu = /*menu ? menu->m_hMenu :*/ NULL;

    cef_window* window = new cef_window();

    ::_HookWindowCreate(window);

    HWND hWnd = ::CreateWindow(szClassname, szWindowTitle, 
                               dwStyles, x, y, width, height, hWndParent, hMenu, gInstance, (LPVOID)window);


    ::_UnHookWindowCreate();

    if (hWnd == NULL) 
    {
        delete window;
        return NULL;
    }

    window->m_hWnd = hWnd;
    return window;
}

LRESULT cef_window::DefaultWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (m_superWndProc)
    {
        return ::CallWindowProc(m_superWndProc, m_hWnd, message, wParam, lParam);
    } 
    else 
    {
        return ::DefWindowProc(m_hWnd, message, wParam, lParam);
    }

}

BOOL cef_window::HandleNonClientDestroy()
{
	WNDPROC superWndProc = WNDPROC(GetWindowLongPtr(m_hWnd, GWLP_WNDPROC));

	RemoveProp(L"CefClientWindowPtr");

	DefaultWindowProc(WM_NCDESTROY, 0, 0);
	
	if ((WNDPROC(GetWindowLongPtr(m_hWnd, GWLP_WNDPROC)) == superWndProc) && (m_superWndProc != NULL))
		SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, reinterpret_cast<INT_PTR>(m_superWndProc));
	
	m_superWndProc = NULL;
	return TRUE;
}

void cef_window::PostNonClientDestory()
{
    m_hWnd = NULL;
}

LRESULT cef_window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
	{
		case WM_NCDESTROY:
			if (HandleNonClientDestroy())
				return 0L;
			break;
	}
	
	LRESULT lr = DefaultWindowProc(message, wParam, lParam);
	
	if (message == WM_NCDESTROY) 
    {
		PostNonClientDestory();
    }

	return lr;
}
