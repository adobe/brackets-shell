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
#include "cefclient.h"
#include "cef_main_window.h"
#include "cef_registry.h"
#include "client_handler.h"
#include "resource.h"
#include "native_menu_model.h"
#include "config.h"

// external
extern HINSTANCE                hInst;
extern CefRefPtr<ClientHandler> g_handler;

// constants
static const wchar_t        kWindowClassname[] = L"CEFCLIENT";
static const wchar_t        kWindowPostionFolder[] = L"Window Position";

static const wchar_t        kPrefLeft[] = L"Left";
static const wchar_t        kPrefTop[] = L"Top";
static const wchar_t        kPrefWidth[] = L"Width";
static const wchar_t        kPrefHeight[] = L"Height";

static const wchar_t        kPrefRestoreLeft[] = L"Restore Left";
static const wchar_t        kPrefRestoreTop[] = L"Restore Top";
static const wchar_t        kPrefRestoreRight[] = L"Restore Right";
static const wchar_t        kPrefRestoreBottom[]  = L"Restore Bottom";
static const wchar_t        kPrefShowState[] = L"Show State";

static const long           kMinWindowWidth = 390;
static const long           kMinWindowHeight = 200;

// Globals
static wchar_t              kCefWindowClosingPropName[] = L"CLOSING";

// The Main Window's window class init helper
ATOM cef_main_window::RegisterWndClass()
{
    static ATOM classAtom = 0;
    if (classAtom == 0) 
    {
        DWORD menuId = IDC_CEFCLIENT;
        WNDCLASSEX wcex;

        ::ZeroMemory (&wcex, sizeof (wcex));
        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style         = CS_SAVEBITS;
        wcex.lpfnWndProc   = ::DefWindowProc;
        wcex.hInstance     = ::hInst;
        wcex.hIcon         = ::LoadIcon(hInst, MAKEINTRESOURCE(IDI_CEFCLIENT));
        wcex.hCursor       = ::LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName  = MAKEINTRESOURCE(menuId);
        wcex.lpszClassName = ::kWindowClassname;
        wcex.hIconSm       = ::LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

        classAtom = RegisterClassEx(&wcex);
    }
    return classAtom;
}

cef_main_window::cef_main_window() 
{
}

cef_main_window::~cef_main_window() 
{
}

// Loads the window title from resource string
LPCWSTR cef_main_window::GetBracketsWindowTitleText()
{
    bool intitialized = false;
    static wchar_t szTitle[100] = L"";

    if (!intitialized) {
        intitialized  = (::LoadString(::hInst, IDS_APP_TITLE, szTitle, _countof(szTitle) - 1) > 0);
    }
    return szTitle;
}

void cef_main_window::EnsureWindowRectVisibility(int& left, int& top, int& width, int& height, int showCmd)
{
    static const int kWindowFrameSize = 8;

    // don't check if we're already letting
    //  Windows determine the window placement
    if (left   == CW_USEDEFAULT &&
        top    == CW_USEDEFAULT &&
        width  == CW_USEDEFAULT &&
        height == CW_USEDEFAULT) {
            return;
    }

    // The virtual display is the bounding rect of all monitors
    // see http://msdn.microsoft.com/en-us/library/dd162729(v=vs.85).aspx

    int xScreen = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
    int yScreen = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
    int cxScreen = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int cyScreen = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // make a copy, we need to adjust the comparison
    //  if it's a maximized window because the OS move the 
    //  origin of the window by 8 pixes to releive the borders 
    //  from the monitor for legacy apps
    int xLeft = left;
    int xTop = top;
    int xWidth = width;
    int xHeight = height;

    if (showCmd == SW_MAXIMIZE) {
        xLeft += kWindowFrameSize;
        xTop += kWindowFrameSize;
        xWidth -= kWindowFrameSize * 2;
        xHeight -= kWindowFrameSize * 2;
    }

    // Make sure the window fits inside the virtual screen.
    // If it doesn't then we let windows decide the window placement
    if (xLeft < xScreen ||
        xTop  < yScreen ||
        xLeft + xWidth > xScreen + cxScreen ||
        xTop + xHeight > yScreen + cyScreen) {

        // something was off-screen so reposition
        //  to the default window placement 
        left   = CW_USEDEFAULT;
        top    = CW_USEDEFAULT;
        width  = CW_USEDEFAULT;
        height = CW_USEDEFAULT;
    }
}


// Create Method.  Call this to create a cef_main_window instance 
BOOL cef_main_window::Create() 
{
    RegisterWndClass();

    int left   = CW_USEDEFAULT;
    int top    = CW_USEDEFAULT;
    int width  = CW_USEDEFAULT;
    int height = CW_USEDEFAULT;

    int showCmd = SW_SHOW;

    LoadWindowRestoreRect(left, top, width, height, showCmd);

    // make sure the window is visible 
    EnsureWindowRectVisibility(left, top, width, height, showCmd);

    DWORD styles =  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_EX_COMPOSITED;

    if (showCmd == SW_MAXIMIZE)
      styles |= WS_MAXIMIZE;

    if (!cef_host_window::Create(::kWindowClassname, GetBracketsWindowTitleText(),
                                styles, left, top, width, height))
    {
        return FALSE;
    }

    RestoreWindowPlacement(showCmd);
    UpdateWindow();

    return TRUE;
}

// PostNcDestroy override 
void cef_main_window::PostNcDestroy()
{
    cef_host_window::PostNcDestroy();

    // We get this notification after the window 
    //  has been destroyed so we need to delete ourself
    delete this;
#ifdef DARK_UI
    // After the main window is destroyed 
    //  do final cleanup...
    DoFinalCleanup();
#endif
}

// Helper to get the browser 
const CefRefPtr<CefBrowser> cef_main_window::GetBrowser()
{
    return g_handler->GetBrowser();
}

// WM_CREATE handler
BOOL cef_main_window::HandleCreate() 
{
    cef_host_window::HandleCreate();

    // Create the single static handler class instance
    g_handler = new ClientHandler();
    g_handler->SetMainHwnd(mWnd);

    RECT rect;

    GetBrowserRect(rect);

    CefWindowInfo info;
    CefBrowserSettings settings;

    settings.web_security = STATE_DISABLED;

    // Initialize window info to the defaults for a child window
    info.SetAsChild(mWnd, rect);

    // Creat the new child browser window
    CefBrowserHost::CreateBrowser(info,
        static_cast<CefRefPtr<CefClient> >(g_handler),
        ::AppGetInitialURL(), settings, NULL);

    return TRUE;
}

// WM_ERASEBKGND handler
BOOL cef_main_window::HandleEraseBackground(HDC hdc)
{
    return (SafeGetCefBrowserHwnd() != NULL);
}

// WM_SETFOCUS handler
BOOL cef_main_window::HandleSetFocus(HWND hLosingFocus)
{
    // Pass focus to the browser window
    CefWindowHandle hwnd = SafeGetCefBrowserHwnd();
    if (hwnd) 
    {
        ::PostMessage(hwnd, WM_SETFOCUS, (WPARAM)hLosingFocus, NULL);
        return TRUE;
    }
    return FALSE;
}

// WM_PAINT handler
BOOL cef_main_window::HandlePaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);

#if defined(DARK_AERO_GLASS) && defined (DARK_UI)
    DoPaintNonClientArea(hdc);
#endif

    EndPaint(&ps);
    return TRUE;
}

// WM_GETMINMAXINFO handler
BOOL cef_main_window::HandleGetMinMaxInfo(LPMINMAXINFO mmi)
{
    mmi->ptMinTrackSize.x = ::kMinWindowWidth;
    mmi->ptMinTrackSize.y = ::kMinWindowHeight;
    return TRUE;
}

// WM_CLOSE handler
BOOL cef_main_window::HandleClose()
{
    SaveWindowRestoreRect();

    CefWindowHandle hwnd = SafeGetCefBrowserHwnd();
    if (hwnd)
    {
        BOOL closing = (BOOL)::GetProp(hwnd, ::kCefWindowClosingPropName);
        if (closing) 
        {
            if (!g_handler->CanCloseBrowser(GetBrowser())) {
                PostMessage(WM_CLOSE, 0 ,0);
                return TRUE;
            }
            ::RemoveProp(hwnd, ::kCefWindowClosingPropName);
        } 
        else 
        {
            g_handler->QuittingApp(true);
            g_handler->DispatchCloseToNextBrowser();
            return TRUE;
        }
    } 
        
    return FALSE;
}

// WM_COMMAND/IDM_EXIT handler
BOOL cef_main_window::HandleExitCommand()
{
    if (g_handler.get()) 
    {
        g_handler->QuittingApp(true);
        g_handler->DispatchCloseToNextBrowser();
    }
    else 
    {
        DestroyWindow();
    }
    return TRUE;
}

// WM_COMMAND handler
BOOL cef_main_window::HandleCommand(UINT commandId)
{
    switch (commandId) 
    {
    case IDM_EXIT:
        return HandleExitCommand();
    default:
        return DoCommand(commandId);
    }
}

// Tear down helper -- 
//  Writes the restore window placement data to the registry
void cef_main_window::SaveWindowRestoreRect()
{
    WINDOWPLACEMENT wp;
    ::ZeroMemory(&wp, sizeof(wp));
    wp.length = sizeof(wp);
     
    if (GetWindowPlacement(&wp))
    {
        // Only save window positions for "restored" and "maximized" states.
        // If window is closed while "minimized", we don't want it to open minimized
        // for next session, so don't update registry so it opens in previous state.
        if (wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_MAXIMIZE)
        {
            RECT rect;
            if (GetWindowRect(&rect))
            {
                ::WriteRegistryInt(::kWindowPostionFolder, ::kPrefLeft,   rect.left);
                ::WriteRegistryInt(::kWindowPostionFolder, ::kPrefTop,    rect.top);
                ::WriteRegistryInt(::kWindowPostionFolder, ::kPrefWidth,  ::RectWidth(rect));
                ::WriteRegistryInt(::kWindowPostionFolder, ::kPrefHeight, ::RectHeight(rect));
            }

            if (wp.showCmd == SW_MAXIMIZE)
            {
                // When window is maximized, we also store the "restore" size
                ::WriteRegistryInt(::kWindowPostionFolder, ::kPrefRestoreLeft,   wp.rcNormalPosition.left);
                ::WriteRegistryInt(::kWindowPostionFolder, ::kPrefRestoreTop,    wp.rcNormalPosition.top);
                ::WriteRegistryInt(::kWindowPostionFolder, ::kPrefRestoreRight,  wp.rcNormalPosition.right);
                ::WriteRegistryInt(::kWindowPostionFolder, ::kPrefRestoreBottom, wp.rcNormalPosition.bottom);
            }

            // Maximize is the only special case we handle
            ::WriteRegistryInt(::kWindowPostionFolder, ::kPrefShowState, (wp.showCmd == SW_MAXIMIZE) ? SW_MAXIMIZE : SW_SHOW);
        }
    }
}

// Initialization helper -- 
//  Loads the restore window placement data from the registry
void cef_main_window::LoadWindowRestoreRect(int& left, int& top, int& width, int& height, int& showCmd)
{
    ::GetRegistryInt(::kWindowPostionFolder, ::kPrefLeft,      NULL, left);
    ::GetRegistryInt(::kWindowPostionFolder, ::kPrefTop,       NULL, top);
    ::GetRegistryInt(::kWindowPostionFolder, ::kPrefWidth,     NULL, width);
    ::GetRegistryInt(::kWindowPostionFolder, ::kPrefHeight,    NULL, height);
    ::GetRegistryInt(::kWindowPostionFolder, ::kPrefShowState, NULL, showCmd);
}

// Initialization helper -- 
//  Loads the Restores data and positions the window in its previously saved state
void cef_main_window::RestoreWindowPlacement(int showCmd)
{
    if (showCmd == SW_MAXIMIZE)
    {
        WINDOWPLACEMENT wp;
        ::ZeroMemory(&wp, sizeof (wp));

        wp.length = sizeof(WINDOWPLACEMENT);

        wp.flags            = 0;
        wp.showCmd          = SW_MAXIMIZE;
        wp.ptMinPosition.x  = -1;
        wp.ptMinPosition.y  = -1;
        wp.ptMaxPosition.x  = -1;
        wp.ptMaxPosition.y  = -1;

        wp.rcNormalPosition.left   = CW_USEDEFAULT;
        wp.rcNormalPosition.top    = CW_USEDEFAULT;
        wp.rcNormalPosition.right  = CW_USEDEFAULT;
        wp.rcNormalPosition.bottom = CW_USEDEFAULT;

        GetRegistryInt(::kWindowPostionFolder, ::kPrefRestoreLeft,   NULL, (int&)wp.rcNormalPosition.left);
        GetRegistryInt(::kWindowPostionFolder, ::kPrefRestoreTop,    NULL, (int&)wp.rcNormalPosition.top);
        GetRegistryInt(::kWindowPostionFolder, ::kPrefRestoreRight,  NULL, (int&)wp.rcNormalPosition.right);
        GetRegistryInt(::kWindowPostionFolder, ::kPrefRestoreBottom, NULL, (int&)wp.rcNormalPosition.bottom);

        ::NormalizeRect(wp.rcNormalPosition);

        int left   = wp.rcNormalPosition.left;
        int top    = wp.rcNormalPosition.top;
        int width  = wp.rcNormalPosition.right - left; 
        int height = wp.rcNormalPosition.bottom - top;

        EnsureWindowRectVisibility(left, top, width, height, SW_SHOWNORMAL);

        // presumably they would all be set to CW_USEDEFAULT
        //  but we check for any out-of-bounds value and
        //  bypass the restore rect as to let Windows decide
        if (left != CW_USEDEFAULT &&
            top != CW_USEDEFAULT && 
            height != CW_USEDEFAULT && 
            width != CW_USEDEFAULT) {

            wp.rcNormalPosition.left = left;
            wp.rcNormalPosition.top = top;

            wp.rcNormalPosition.right = wp.rcNormalPosition.left + width;
            wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + height;
        
            // This returns FALSE on failure but not sure what we could do in that case
            SetWindowPlacement(&wp);
        }
    }

    ShowWindow(showCmd);
}

// WM_COPYDATA handler
BOOL cef_main_window::HandleCopyData(HWND, PCOPYDATASTRUCT lpCopyData) 
{
    if ((lpCopyData) && (lpCopyData->dwData == ID_WM_COPYDATA_SENDOPENFILECOMMAND) && (lpCopyData->cbData > 0)) {
        // another Brackets instance requests that we open the given files/folders
        std::wstring wstrFilename = (LPCWSTR)lpCopyData->lpData;
        std::wstring wstrFileArray = L"[";
        bool hasMultipleFiles = false;
        
        if (wstrFilename.find('"') != std::wstring::npos) {
            if (wstrFilename.find(L"\" ") != std::wstring::npos ||
               (wstrFilename.front() != '"' || wstrFilename.back() != '"')) {
                hasMultipleFiles = true;
            }
        } else {
            hasMultipleFiles = (wstrFilename.find(L" ") != std::wstring::npos);
        }

        if (hasMultipleFiles) {
            std::size_t curFilePathEnd1 = wstrFilename.find(L" ");
            std::size_t curFilePathEnd2 = wstrFilename.find(L"\" ");
            std::size_t nextQuoteIndex  = wstrFilename.find(L"\"");
 
            while ((nextQuoteIndex == 0 && curFilePathEnd2 != std::wstring::npos) || 
                   (nextQuoteIndex != 0 && curFilePathEnd1 != std::wstring::npos)) {

                if (nextQuoteIndex == 0 && curFilePathEnd2 != std::wstring::npos) {
                    // Appending a file path that is already wrapped in double-quotes.
                    wstrFileArray += (wstrFilename.substr(0, curFilePathEnd2 + 1) + L",");

                    // Strip the current file path and move index to next file path.
                    wstrFilename = wstrFilename.substr(curFilePathEnd2 + 2);
                } else {
                    // Explicitly wrap a file path in double-quotes and append it to the file array.
                    wstrFileArray += (L"\"" + wstrFilename.substr(0, curFilePathEnd1) + L"\",");

                    // Strip the current file path and move index to next file path.
                    wstrFilename = wstrFilename.substr(curFilePathEnd1 + 1);
                }

                curFilePathEnd1 = wstrFilename.find(L" ");
                curFilePathEnd2 = wstrFilename.find(L"\" ");
                nextQuoteIndex = wstrFilename.find(L"\"");
            }
        }

        // Add the last file or the only file into the file array.
        if (wstrFilename.front() == '"' && wstrFilename.back() == '"') {
            wstrFileArray += wstrFilename;
        } else if (wstrFilename.length()) {
            wstrFileArray += (L"\"" + wstrFilename + L"\"");
        }
        wstrFileArray += L"]";

        g_handler->SendOpenFileCommand(g_handler->GetBrowser(), CefString(wstrFileArray.c_str()));
        return TRUE;
    }

    return FALSE;
}

// EnumWindowsProc callback function
//  - searches for an already running Brackets application window
BOOL CALLBACK cef_main_window::FindSuitableBracketsInstanceHelper(HWND hwnd, LPARAM lParam)
{
    if (lParam == NULL)
        return FALSE;

    // check for the Brackets application window by class name and title
    WCHAR cName[MAX_PATH+1] = {0}, cTitle[MAX_PATH+1] = {0};
    ::GetClassName(hwnd, cName, MAX_PATH);
    ::GetWindowText(hwnd, cTitle, MAX_PATH);

    if ((wcscmp(cName, ::kWindowClassname) == 0) && (wcsstr(cTitle, GetBracketsWindowTitleText()) != 0)) {
        // found an already running instance of Brackets.  Now, check that that window
        //   isn't currently disabled (eg. modal dialog).  If it is keep searching.
        if ((::GetWindowLong(hwnd, GWL_STYLE) & WS_DISABLED) == 0) {
            //return the window handle and stop searching
            *(HWND*)lParam = hwnd;
            return FALSE;
        }
    }

    return TRUE;    // otherwise, continue searching
}

// Locates a top level instance of Brackets already running
HWND cef_main_window::FindFirstTopLevelInstance()
{
    HWND hFirstInstanceWnd = NULL;
    ::EnumWindows(FindSuitableBracketsInstanceHelper, (LPARAM)&hFirstInstanceWnd);
    return hFirstInstanceWnd;
}

// WM_SIZE handler
BOOL cef_main_window::HandleSize(BOOL bMinimize)
{
    CefWindowHandle hwnd = SafeGetCefBrowserHwnd();
    if (!hwnd) 
        return FALSE;

    RECT rect;
    GetBrowserRect(rect);

    // Minimizing the window to 0x0 which causes our layout to go all
    // screwy, so we just ignore it.
    if (!bMinimize) 
    {
        HDWP hdwp = ::BeginDeferWindowPos(1);
        hdwp = ::DeferWindowPos(hdwp, hwnd, NULL, rect.left, rect.top, ::RectWidth(rect), ::RectHeight(rect), SWP_NOZORDER);
        ::EndDeferWindowPos(hdwp);
    }

    return FALSE;
}

// WindowProc -- Dispatches and routes window messages
LRESULT cef_main_window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
    case WM_CREATE:
        if (HandleCreate())
            return 0L;
        break;
    case WM_ERASEBKGND:
        if (HandleEraseBackground((HDC)wParam))
            return 1L;
        break;
    case WM_SETFOCUS:
        if (HandleSetFocus((HWND)wParam))
            return 0L;
        break;
    case WM_PAINT:
        if (HandlePaint())
            return 0L;
        break;
    case WM_DESTROY:
        return 0L; // Do not handle the destroy here.
        break;
    case WM_CLOSE:
        if (HandleClose())
            return 0L;
        break;
    case WM_SIZE:
        if (HandleSize(wParam == SIZE_MINIMIZED))
            return 0L;
        break;
    case WM_COMMAND:
        if (HandleCommand(LOWORD(wParam)))
            return 0L;
        break;
    case WM_COPYDATA:
        if (HandleCopyData((HWND)wParam, (PCOPYDATASTRUCT)lParam))
            return 0L;
        break;
    }

    LRESULT lr = cef_host_window::WindowProc(message, wParam, lParam);

    switch (message) 
    {
    case WM_GETMINMAXINFO:
        HandleGetMinMaxInfo((LPMINMAXINFO) lParam);
        break;
    }

    return lr;
}
