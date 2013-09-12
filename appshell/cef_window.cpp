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

extern HINSTANCE hInst;   // current instance

static const wchar_t gCefClientWindowPropName[] = L"CefClientWindowPtr";

struct HookData {
    HookData() 
    {
        this->Reset();
    }
    void Reset()
    {
        mOldHook = NULL;
        mWindow = NULL;        
    }

    HHOOK       mOldHook;
    cef_window* mWindow;
} gHookData;




cef_window::cef_window(void) :
    mWnd(NULL),
    mSuperWndProc(NULL)
{
}


cef_window::~cef_window(void)
{
}

static LRESULT CALLBACK _HookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code != HCBT_CREATEWND)
        return CallNextHookEx(gHookData.mOldHook, code, wParam, lParam);
    
    LPCREATESTRUCT lpcs = ((LPCBT_CREATEWND)lParam)->lpcs;

    HHOOK nextHook = gHookData.mOldHook;

    if (lpcs->lpCreateParams && lpcs->lpCreateParams == (LPVOID)gHookData.mWindow) 
    {
        HWND hWnd = (HWND)wParam;
        gHookData.mWindow->SubclassWindow(hWnd);
        // Rest the hook data here since we've already hooked this window
        //  this allows for other windows to be created in the WM_CREATE handlers
        //  of subclassed windows
        gHookData.Reset();
    }

    return CallNextHookEx(nextHook, code, wParam, lParam);
}

static void _HookWindowCreate(cef_window* window)
{
    // can only hook one creation at a time
    if (gHookData.mOldHook || gHookData.mWindow) 
        return;

    gHookData.mOldHook = ::SetWindowsHookEx(WH_CBT, _HookProc, NULL, ::GetCurrentThreadId());
    gHookData.mWindow = window;
}


static void _UnHookWindowCreate()
{
    gHookData.Reset();
}

static LRESULT CALLBACK _WindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  cef_window* window = (cef_window*)::GetProp(hWnd, ::gCefClientWindowPropName);
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
    if (::GetProp(hWnd, ::gCefClientWindowPropName) != NULL) 
        return false;
    mWnd = hWnd;
    mSuperWndProc = (WNDPROC)SetWindowLongPtr(GWLP_WNDPROC, (LONG_PTR)&_WindowProc);
    SetProp(::gCefClientWindowPropName, (HANDLE)this);
    return true;
}

static HWND _xxCreateWindow(LPCTSTR szClassname, LPCTSTR szWindowTitle, DWORD dwStyles, int x, int y, int width, int height, HWND hWndParent, HMENU hMenu, cef_window* window)
{
    ::_HookWindowCreate(window);

    HWND result = CreateWindow(szClassname, szWindowTitle, 
                               dwStyles, x, y, width, height, hWndParent, hMenu, ::hInst, (LPVOID)window);


    ::_UnHookWindowCreate();
    return result;
}

BOOL cef_window::Create(LPCTSTR szClassname, LPCTSTR szWindowTitle, DWORD dwStyles, int x, int y, int width, int height, cef_window* parent/*=NULL*/, cef_menu* menu/*=NULL*/)
{
    HWND hWndParent = parent ? parent->mWnd : NULL;
    HMENU hMenu = /*menu ? menu->m_hMenu :*/ NULL;

    HWND hWndThis = _xxCreateWindow(szClassname, szWindowTitle, 
                               dwStyles, x, y, width, height, hWndParent, hMenu, this);


    return hWndThis && hWndThis == mWnd;
}

LRESULT cef_window::DefaultWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (mSuperWndProc)
    {
        return ::CallWindowProc(mSuperWndProc, mWnd, message, wParam, lParam);
    } 
    else 
    {
        return ::DefWindowProc(mWnd, message, wParam, lParam);
    }

}

BOOL cef_window::HandleNcDestroy()
{
    WNDPROC superWndProc = WNDPROC(GetWindowLongPtr(GWLP_WNDPROC));

    RemoveProp(::gCefClientWindowPropName);

    DefaultWindowProc(WM_NCDESTROY, 0, 0);
	
    if ((WNDPROC(GetWindowLongPtr(GWLP_WNDPROC)) == superWndProc) && (mSuperWndProc != NULL))
	    SetWindowLongPtr(GWLP_WNDPROC, reinterpret_cast<INT_PTR>(mSuperWndProc));
	
    mSuperWndProc = NULL;
    return TRUE;
}

void cef_window::PostNcDestory()
{
    mWnd = NULL;
}

LRESULT cef_window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
	    case WM_NCDESTROY:
		    if (HandleNcDestroy())
			    return 0L;
		    break;
    }
	
    LRESULT lr = DefaultWindowProc(message, wParam, lParam);
	
    if (message == WM_NCDESTROY) 
    {
	    PostNcDestory();
    }

    return lr;
}

void cef_window::ScreenToNonClient(LPRECT r) const
{
    WINDOWINFO wi ;
    ::ZeroMemory ( &wi, sizeof ( wi ) ) ;
    wi.cbSize = sizeof ( wi ) ;
    GetWindowInfo ( &wi ) ;

    int height = ::RectHeight(*r);
    int width = ::RectWidth(*r);

    r->top = r->top - wi.rcWindow.top;
    r->left = r->left - wi.rcWindow.left;
    r->bottom = r->top + height;
    r->right = r->left + width;
}

// Computes the client rect relative to the Window Rect
//	Used to compute the clipping region, etc...
void cef_window::ComputeLogicalClientRect(RECT& rectClient)
{
    WINDOWINFO wi ;
    ::ZeroMemory (&wi, sizeof ( wi ) ) ;
    wi.cbSize = sizeof ( wi ) ;
    GetWindowInfo ( &wi ) ;

    ::CopyRect(&rectClient, &wi.rcClient);

    int height = ::RectHeight(wi.rcClient);
    int width = ::RectWidth(wi.rcClient);

    rectClient.top = wi.rcClient.top - wi.rcWindow.top;
    rectClient.left = wi.rcClient.left - wi.rcWindow.left;
    rectClient.bottom = rectClient.top + height;
    rectClient.right = rectClient.left + width;
}

void cef_window::ComputeLogicalWindowRect (RECT& rectWindow)
{
    RECT wr;
    GetWindowRect (&wr);

    ::SetRectEmpty(&rectWindow);

    rectWindow.bottom = ::RectHeight(wr);
    rectWindow.right = ::RectWidth(wr);
}


void cef_window::NonClientToScreen(LPRECT r) const
{
    RECT wr;
    GetWindowRect (&wr);

    r->top += wr.top;
    r->left += wr.left;
    r->bottom += wr.top;
    r->right += wr.left;
}

void cef_window::ScreenToClient(LPRECT lpRect) const
{
	::ScreenToClient(mWnd, (LPPOINT)lpRect);
	::ScreenToClient(mWnd, ((LPPOINT)lpRect)+1);
	if (GetExStyle() & WS_EX_LAYOUTRTL)
		::RectSwapLeftRight(*lpRect);
}

void cef_window::ClientToScreen(LPRECT lpRect) const
{
	::ClientToScreen(mWnd, (LPPOINT)lpRect);
	::ClientToScreen(mWnd, ((LPPOINT)lpRect)+1);
	if (GetExStyle() & WS_EX_LAYOUTRTL)
		::RectSwapLeftRight(*lpRect);
}

// Turns mouse tracking on or off
//	we track non-client mouse events when the user moves the
//	mouse over one of our buttons. We want a WM_NCMOUSELEAVE
//	message so we can redraw the buttons in the non-hovered 
//	state this method tells windows we want the WM_NCMOUSELEAVE
//	message.  It can tell it that we don't care anymore too, 
//	which is nice...
BOOL cef_window::TrackNonClientMouseEvents( bool track/*=true*/) 
{
	TRACKMOUSEEVENT tme ;
	::ZeroMemory( &tme, sizeof ( tme ) ) ;

	tme.cbSize = sizeof ( tme ) ;

	tme.dwFlags = TME_QUERY ;
	tme.hwndTrack = mWnd ;
	 
	::TrackMouseEvent ( &tme ) ;

	/// i.e. if we're currently tracking and the caller
	//	 wanted to track or if we're not currently tracking and
	//	 the caller wanted to turn off tracking then just bail
	if ((( tme.dwFlags & TME_LEAVE ) == TME_LEAVE ) == track ) 
			return FALSE; // nothing to do...

	tme.dwFlags = TME_LEAVE|TME_NONCLIENT;

	if ( !track ) 
		tme.dwFlags |= TME_CANCEL;

	// The previous call to TrackMouseEvent destroys the hwndTrack 
	//	and cbSize members so we have to re-initialize them.
	tme.hwndTrack = mWnd ;
	tme.cbSize = sizeof ( tme ) ;

	return ::TrackMouseEvent ( &tme );
}