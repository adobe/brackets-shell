// Copyright (c) 2010 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient.h"
#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#include <MMSystem.h>
#include <sstream>
#include <string>
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "client_handler.h"
#include "config.h"
#include "resource.h"
#include "string_util.h"
#include "client_switches.h"
#include "native_menu_model.h"
#include "appshell_node_process.h"

#include <algorithm>
#include <ShellAPI.h>
#include <ShlObj.h>

#define MAX_LOADSTRING 100

#ifdef SHOW_TOOLBAR_UI
#define MAX_URL_LENGTH  255
#define BUTTON_WIDTH 72
#define URLBAR_HEIGHT  24
#endif // SHOW_TOOLBAR_UI

#define CLOSING_PROP L"CLOSING"

// Global Variables:
DWORD g_appStartupTime;
HINSTANCE hInst;   // current instance
HACCEL hAccelTable;
HWND hWndMain;
std::wstring gFilesToOpen; // Filenames passed as arguments to app
TCHAR szTitle[MAX_LOADSTRING];  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];  // the main window class name
char szWorkingDir[MAX_PATH];  // The current working directory

TCHAR szInitialUrl[MAX_PATH] = {0};

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance, const cef_string_t& locale);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

#if defined(OS_WIN)
// Add Common Controls to the application manifest because it's required to
// support the default tooltip implementation.
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")  // NOLINT(whitespace/line_length)
#endif

// Registry access functions
void EnsureTrailingSeparator(LPWSTR pRet);
void GetKey(LPCWSTR pBase, LPCWSTR pGroup, LPCWSTR pApp, LPCWSTR pFolder, LPWSTR pRet);
bool GetRegistryInt(LPCWSTR pFolder, LPCWSTR pEntry, int* pDefault, int& ret);
bool WriteRegistryInt (LPCWSTR pFolder, LPCWSTR pEntry, int val);

// Registry key strings
#define PREF_APPSHELL_BASE		L"Software"
#define PREF_WINPOS_FOLDER		L"Window Position"
#define PREF_LEFT				L"Left"
#define PREF_TOP				L"Top"
#define PREF_WIDTH				L"Width"
#define PREF_HEIGHT				L"Height"
#define PREF_RESTORE_LEFT		L"Restore Left"
#define PREF_RESTORE_TOP		L"Restore Top"
#define PREF_RESTORE_RIGHT		L"Restore Right"
#define PREF_RESTORE_BOTTOM		L"Restore Bottom"
#define PREF_SHOWSTATE			L"Show State"

// Window state functions
void SaveWindowRect(HWND hWnd);
void RestoreWindowRect(int& left, int& top, int& width, int& height, int& showCmd);
void RestoreWindowPlacement(HWND hWnd, int showCmd);

bool IsFilename(const std::wstring& str) {
    // See if we can access the passed in value
    return (GetFileAttributes(str.c_str()) != INVALID_FILE_ATTRIBUTES);
}

std::wstring GetFilenamesFromCommandLine() {
    std::wstring result = L"[]";

    if (AppGetCommandLine()->HasArguments()) {
        bool firstEntry = true;
        std::vector<CefString> args;
        AppGetCommandLine()->GetArguments(args);
        std::vector<CefString>::iterator iterator;

        result = L"[";
        for (iterator = args.begin(); iterator != args.end(); iterator++) {
            std::wstring argument = (*iterator).ToWString();
            if (IsFilename(argument)) {
                if (!firstEntry) {
                    result += L",";
                }
                firstEntry = false;
                result += L"\"" + argument + L"\"";
            }
        }
        result += L"]";
    }

    return result;
}

// Program entry point function.
int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  g_appStartupTime = timeGetTime();

  CefMainArgs main_args(hInstance);
  CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app.get());
  if (exit_code >= 0)
    return exit_code;

  // Retrieve the current working directory.
  if (_getcwd(szWorkingDir, MAX_PATH) == NULL)
    szWorkingDir[0] = 0;

  // Parse command line arguments. The passed in values are ignored on Windows.
  AppInitCommandLine(0, NULL);

  CefSettings settings;

  // Populate the settings based on command line arguments.
  AppGetSettings(settings, app);

  // Check command
  if (CefString(&settings.cache_path).length() == 0) {
	  CefString(&settings.cache_path) = AppGetCachePath();
  }

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get());

  // Initialize global strings
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadString(hInstance, IDC_CEFCLIENT, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance, *(app->GetCurrentLanguage().GetStruct()));
 
  CefRefPtr<CefCommandLine> cmdLine = AppGetCommandLine();
  if (cmdLine->HasSwitch(cefclient::kStartupPath)) {
	  wcscpy(szInitialUrl, cmdLine->GetSwitchValue(cefclient::kStartupPath).c_str());
  }
  else {
	// If the shift key is not pressed, look for the index.html file 
	if (GetAsyncKeyState(VK_SHIFT) == 0) {
	// Get the full pathname for the app. We look for the index.html
	// file relative to this location.
	wchar_t appPath[MAX_PATH];
	wchar_t *pathRoot;
	GetModuleFileName(NULL, appPath, MAX_PATH);

	// Strip the .exe filename (and preceding "\") from the appPath
	// and store in pathRoot
	pathRoot = wcsrchr(appPath, '\\');

	// Look for .\dev\src\index.html first
	wcscpy(pathRoot, L"\\dev\\src\\index.html");

	// If the file exists, use it
	if (GetFileAttributes(appPath) != INVALID_FILE_ATTRIBUTES) {
		wcscpy(szInitialUrl, appPath);
	}

	if (!wcslen(szInitialUrl)) {
		// Look for .\www\index.html next
		wcscpy(pathRoot, L"\\www\\index.html");
		if (GetFileAttributes(appPath) != INVALID_FILE_ATTRIBUTES) {
		wcscpy(szInitialUrl, appPath);
		}
	}
	}
  }

  if (!wcslen(szInitialUrl)) {
      // If we got here, either the startup file couldn't be found, or the user pressed the
      // shift key while launching. Prompt to select the index.html file.
      OPENFILENAME ofn = {0};
      ofn.lStructSize = sizeof(ofn);
      ofn.lpstrFile = szInitialUrl;
      ofn.nMaxFile = MAX_PATH;
      ofn.lpstrFilter = L"Web Files\0*.htm;*.html\0\0";
      ofn.lpstrTitle = L"Please select the " APP_NAME L" index.html file.";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

      if (!GetOpenFileName(&ofn)) {
        // User cancelled, exit the app
        CefShutdown();
        return 0;
      }
  }

  // Perform application initialization
  if (!InitInstance (hInstance, nCmdShow))
    return FALSE;

  // Start the node server process
  startNodeProcess();

  gFilesToOpen = GetFilenamesFromCommandLine();

  int result = 0;

  if (!settings.multi_threaded_message_loop) {
    // Run the CEF message loop. This function will block until the application
    // recieves a WM_QUIT message.
    CefRunMessageLoop();
  } else {
    MSG msg;

    // Run the application message loop.
    while (GetMessage(&msg, NULL, 0, 0)) {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    result = static_cast<int>(msg.wParam);
  }

  OnBeforeShutdown();

  // Shut down CEF.
  CefShutdown();

  return result;
}

// Add trailing separator, if necessary
void EnsureTrailingSeparator(LPWSTR pRet)
{
	if (!pRet)
		return;

	int len = wcslen(pRet);
	if (len > 0 && wcscmp(&(pRet[len-1]), L"\\") != 0)
	{
		wcscat(pRet, L"\\");
	}
}

// Helper method to build Registry Key string
void GetKey(LPCWSTR pBase, LPCWSTR pGroup, LPCWSTR pApp, LPCWSTR pFolder, LPWSTR pRet)
{
	// Check for required params
	ASSERT(pBase && pApp && pRet);
	if (!pBase || !pApp || !pRet)
		return;

	// Base
	wcscpy(pRet, pBase);

	// Group (optional)
	if (pGroup && (pGroup[0] != '\0'))
	{
		EnsureTrailingSeparator(pRet);
		wcscat(pRet, pGroup);
	}

	// App name
	EnsureTrailingSeparator(pRet);
	wcscat(pRet, pApp);

	// Folder (optional)
	if (pFolder && (pFolder[0] != '\0'))
	{
		EnsureTrailingSeparator(pRet);
		wcscat(pRet, pFolder);
	}
}

// get integer value from registry key
// caller can either use return value to determine success/fail, or pass a default to be used on fail
bool GetRegistryInt(LPCWSTR pFolder, LPCWSTR pEntry, int* pDefault, int& ret)
{
	HKEY hKey;
	bool result = false;

	wchar_t key[MAX_PATH];
	key[0] = '\0';
	GetKey(PREF_APPSHELL_BASE, GROUP_NAME, APP_NAME, pFolder, (LPWSTR)&key);

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, (LPCWSTR)key, 0, KEY_READ, &hKey))
	{
		DWORD dwValue = 0;
		DWORD dwType = 0;
		DWORD dwCount = sizeof(DWORD);
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, pEntry, NULL, &dwType, (LPBYTE)&dwValue, &dwCount))
		{
			result = true;
			ASSERT(dwType == REG_DWORD);
			ASSERT(dwCount == sizeof(dwValue));
			ret = (int)dwValue;
		}
		RegCloseKey(hKey);
	}

	if (!result)
	{
		// couldn't read value, so use default, if specified
		if (pDefault)
			ret = *pDefault;
	}

	return result;
}

// write integer value to registry key
bool WriteRegistryInt(LPCWSTR pFolder, LPCWSTR pEntry, int val)
{
	HKEY hKey;
	bool result = false;

	wchar_t key[MAX_PATH];
	key[0] = '\0';
	GetKey(PREF_APPSHELL_BASE, GROUP_NAME, APP_NAME, pFolder, (LPWSTR)&key);

	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, (LPCWSTR)key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
	{
		DWORD dwCount = sizeof(int);
		if (ERROR_SUCCESS == RegSetValueEx(hKey, pEntry, 0, REG_DWORD, (LPBYTE)&val, dwCount))
			result = true;
		RegCloseKey(hKey);
	}

	return result;
}

void SaveWindowRect(HWND hWnd)
{
	// Save position of active window
	if (!hWnd)
		return;

	WINDOWPLACEMENT wp;
	memset(&wp, 0, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);

	if (GetWindowPlacement(hWnd, &wp))
	{
		// Only save window positions for "restored" and "maximized" states.
		// If window is closed while "minimized", we don't want it to open minimized
		// for next session, so don't update registry so it opens in previous state.
		if (wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_MAXIMIZE)
		{
			RECT rect;
			if (GetWindowRect(hWnd, &rect))
			{
				WriteRegistryInt(PREF_WINPOS_FOLDER, PREF_LEFT,   rect.left);
				WriteRegistryInt(PREF_WINPOS_FOLDER, PREF_TOP,    rect.top);
				WriteRegistryInt(PREF_WINPOS_FOLDER, PREF_WIDTH,  rect.right - rect.left);
				WriteRegistryInt(PREF_WINPOS_FOLDER, PREF_HEIGHT, rect.bottom - rect.top);
			}

			if (wp.showCmd == SW_MAXIMIZE)
			{
				// When window is maximized, we also store the "restore" size
				WriteRegistryInt(PREF_WINPOS_FOLDER, PREF_RESTORE_LEFT,   wp.rcNormalPosition.left);
				WriteRegistryInt(PREF_WINPOS_FOLDER, PREF_RESTORE_TOP,    wp.rcNormalPosition.top);
				WriteRegistryInt(PREF_WINPOS_FOLDER, PREF_RESTORE_RIGHT,  wp.rcNormalPosition.right);
				WriteRegistryInt(PREF_WINPOS_FOLDER, PREF_RESTORE_BOTTOM, wp.rcNormalPosition.bottom);
			}

			// Maximize is the only special case we handle
			WriteRegistryInt(PREF_WINPOS_FOLDER, PREF_SHOWSTATE,
							 (wp.showCmd == SW_MAXIMIZE) ? SW_MAXIMIZE : SW_SHOW);
		}
	}
}

void RestoreWindowRect(int& left, int& top, int& width, int& height, int& showCmd)
{
	// Start with Registry data
	GetRegistryInt(PREF_WINPOS_FOLDER, PREF_LEFT,		NULL, left);
	GetRegistryInt(PREF_WINPOS_FOLDER, PREF_TOP,		NULL, top);
	GetRegistryInt(PREF_WINPOS_FOLDER, PREF_WIDTH,		NULL, width);
	GetRegistryInt(PREF_WINPOS_FOLDER, PREF_HEIGHT,		NULL, height);
	GetRegistryInt(PREF_WINPOS_FOLDER, PREF_SHOWSTATE,  NULL, showCmd);

	// If window size has changed, we may need to alter window size
	HMONITOR	hMonitor;
	MONITORINFO	mi;
	RECT		rcOrig, rcMax;

	// Get the nearest monitor to the passed rect
	rcOrig.left   = left;
	rcOrig.top    = top;
	rcOrig.right  = left + width;
	rcOrig.bottom = top  + height;
	hMonitor = MonitorFromRect(&rcOrig, MONITOR_DEFAULTTONEAREST);

	// Get the monitor rect
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);
	rcMax = mi.rcMonitor;

	if (showCmd == SW_MAXIMIZE)
	{
		// For maximized case, just use monitor dimensions
		left   = rcMax.left;
		top    = rcMax.top;
		width  = rcMax.right  - rcMax.left;
		height = rcMax.bottom - rcMax.top;
	}
	else
	{
		// For non-maximized case, adjust window to fit on screen

		// make sure width fits
		int rcMaxWidth = static_cast<int>(rcMax.right - rcMax.left);
		if (width > rcMaxWidth)
			width = rcMaxWidth;

		// make sure left is on screen
		if (left < rcMax.left)
			left = static_cast<int>(rcMax.left);

		// make sure right is on screen
		if ((left + width) > rcMax.right)
			left = static_cast<int>(rcMax.right) - width;

		// make sure height fits
		int rcMaxHeight = static_cast<int>(rcMax.bottom - rcMax.top);
		if (height > rcMaxHeight)
			height = rcMaxHeight;

		// make sure top is on screen
		if (top < rcMax.top)
			top = static_cast<int>(rcMax.top);

		// make sure bottom is on screen
		if ((top + height) > rcMax.bottom)
			top = static_cast<int>(rcMax.bottom) - height;
	}
}

void RestoreWindowPlacement(HWND hWnd, int showCmd)
{
	if (!hWnd)
		return;

	// If window is maximized, set the "restore" window position
	if (showCmd == SW_MAXIMIZE)
	{
		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);

		wp.flags	= 0;
		wp.showCmd	= SW_MAXIMIZE;
		wp.ptMinPosition.x	= -1;
		wp.ptMinPosition.y	= -1;
		wp.ptMaxPosition.x	= -1;
		wp.ptMaxPosition.y	= -1;

		wp.rcNormalPosition.left	= CW_USEDEFAULT;
		wp.rcNormalPosition.top		= CW_USEDEFAULT;
		wp.rcNormalPosition.right	= CW_USEDEFAULT;
		wp.rcNormalPosition.bottom	= CW_USEDEFAULT;

		GetRegistryInt(PREF_WINPOS_FOLDER, PREF_RESTORE_LEFT,	NULL, (int&)wp.rcNormalPosition.left);
		GetRegistryInt(PREF_WINPOS_FOLDER, PREF_RESTORE_TOP,	NULL, (int&)wp.rcNormalPosition.top);
		GetRegistryInt(PREF_WINPOS_FOLDER, PREF_RESTORE_RIGHT,	NULL, (int&)wp.rcNormalPosition.right);
		GetRegistryInt(PREF_WINPOS_FOLDER, PREF_RESTORE_BOTTOM,	NULL, (int&)wp.rcNormalPosition.bottom);

		// This returns an error code, but not sure what we could do on an error
		SetWindowPlacement(hWnd, &wp);
	}

	ShowWindow(hWnd, showCmd);
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this
//    function so that the application will get 'well formed' small icons
//    associated with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance, const cef_string_t& locale) {

  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = WndProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = 0;
  wcex.hInstance     = hInstance;
  wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CEFCLIENT));
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wcex.lpszMenuName  = NULL;
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

  return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
  hInst = hInstance;  // Store instance handle in our global variable

  // TODO: test this cases:
  // - window in secondary monitor when shutdown, disconnect secondary monitor, restart

  int left   = CW_USEDEFAULT;
  int top    = CW_USEDEFAULT;
  int width  = CW_USEDEFAULT;
  int height = CW_USEDEFAULT;
  int showCmd = SW_SHOW;
  RestoreWindowRect(left, top, width, height, showCmd);

  DWORD styles = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
  if (showCmd == SW_MAXIMIZE)
	  styles |= WS_MAXIMIZE;
  hWndMain = CreateWindow(szWindowClass, szTitle, styles,
                      left, top, width, height, NULL, NULL, hInstance, NULL);

  if (!hWndMain)
    return FALSE;

  DragAcceptFiles(hWndMain, TRUE);
  RestoreWindowPlacement(hWndMain, showCmd);
  UpdateWindow(hWndMain);
\
  return TRUE;
}

LRESULT HandleDropFiles(HDROP hDrop, CefRefPtr<ClientHandler> handler, CefRefPtr<CefBrowser> browser) {
    UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
    for (UINT i = 0; i < fileCount; i++) {
        wchar_t filename[MAX_PATH];
        DragQueryFile(hDrop, i, filename, MAX_PATH);
        std::wstring pathStr(filename);
        replace(pathStr.begin(), pathStr.end(), '\\', '/');
        handler->SendOpenFileCommand(browser, CefString(pathStr));
    }
    return 0;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  static HWND backWnd = NULL, forwardWnd = NULL, reloadWnd = NULL,
      stopWnd = NULL, editWnd = NULL;
  static WNDPROC editWndOldProc = NULL;

  // Static members used for the find dialog.
  static FINDREPLACE fr;
  static WCHAR szFindWhat[80] = {0};
  static WCHAR szLastFindWhat[80] = {0};
  static bool findNext = false;
  static bool lastMatchCase = false;

  int wmId, wmEvent;
  PAINTSTRUCT ps;
  HDC hdc;

#ifdef SHOW_TOOLBAR_UI
  if (hWnd == editWnd) {
    // Callback for the edit window
    switch (message) {
    case WM_CHAR:
      if (wParam == VK_RETURN && g_handler.get()) {
        // When the user hits the enter key load the URL
        CefRefPtr<CefBrowser> browser = g_handler->GetBrowser();
        wchar_t strPtr[MAX_URL_LENGTH+1] = {0};
        *((LPWORD)strPtr) = MAX_URL_LENGTH;
        LRESULT strLen = SendMessage(hWnd, EM_GETLINE, 0, (LPARAM)strPtr);
        if (strLen > 0) {
          strPtr[strLen] = 0;
          browser->GetMainFrame()->LoadURL(strPtr);
        }

        return 0;
      }
    }
    return (LRESULT)CallWindowProc(editWndOldProc, hWnd, message, wParam,
                                   lParam);
  } else
#endif // SHOW_TOOLBAR_UI
  {
    // Callback for the main window
    switch (message) {
    case WM_CREATE: {
      // Create the single static handler class instance
      g_handler = new ClientHandler();
      g_handler->SetMainHwnd(hWnd);

      // Create the child windows used for navigation
      RECT rect;
      int x = 0;

      GetClientRect(hWnd, &rect);

#ifdef SHOW_TOOLBAR_UI
      backWnd = CreateWindow(L"BUTTON", L"Back",
                              WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON
                              | WS_DISABLED, x, 0, BUTTON_WIDTH, URLBAR_HEIGHT,
                              hWnd, (HMENU) IDC_NAV_BACK, hInst, 0);
      x += BUTTON_WIDTH;

      forwardWnd = CreateWindow(L"BUTTON", L"Forward",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON
                                | WS_DISABLED, x, 0, BUTTON_WIDTH,
                                URLBAR_HEIGHT, hWnd, (HMENU) IDC_NAV_FORWARD,
                                hInst, 0);
      x += BUTTON_WIDTH;

      reloadWnd = CreateWindow(L"BUTTON", L"Reload",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON
                                | WS_DISABLED, x, 0, BUTTON_WIDTH,
                                URLBAR_HEIGHT, hWnd, (HMENU) IDC_NAV_RELOAD,
                                hInst, 0);
      x += BUTTON_WIDTH;

      stopWnd = CreateWindow(L"BUTTON", L"Stop",
                              WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON
                              | WS_DISABLED, x, 0, BUTTON_WIDTH, URLBAR_HEIGHT,
                              hWnd, (HMENU) IDC_NAV_STOP, hInst, 0);
      x += BUTTON_WIDTH;

      editWnd = CreateWindow(L"EDIT", 0,
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT |
                              ES_AUTOVSCROLL | ES_AUTOHSCROLL| WS_DISABLED,
                              x, 0, rect.right - BUTTON_WIDTH * 4,
                              URLBAR_HEIGHT, hWnd, 0, hInst, 0);

      // Assign the edit window's WNDPROC to this function so that we can
      // capture the enter key
      editWndOldProc =
          reinterpret_cast<WNDPROC>(GetWindowLongPtr(editWnd, GWLP_WNDPROC));
      SetWindowLongPtr(editWnd, GWLP_WNDPROC,
          reinterpret_cast<LONG_PTR>(WndProc));
      g_handler->SetEditHwnd(editWnd);
      g_handler->SetButtonHwnds(backWnd, forwardWnd, reloadWnd, stopWnd);

      rect.top += URLBAR_HEIGHT;
#endif // SHOW_TOOLBAR_UI

      CefWindowInfo info;
      CefBrowserSettings settings;

      // Populate the settings based on command line arguments.
      AppGetBrowserSettings(settings);

      settings.file_access_from_file_urls_allowed = true;
      settings.universal_access_from_file_urls_allowed = true;

      // Initialize window info to the defaults for a child window
      info.SetAsChild(hWnd, rect);

      // Creat the new child browser window
      CefBrowserHost::CreateBrowser(info,
          static_cast<CefRefPtr<CefClient> >(g_handler),
          szInitialUrl, settings);

      return 0;
    }

    case WM_COMMAND: {
      CefRefPtr<CefBrowser> browser;
      if (g_handler.get())
        browser = g_handler->GetBrowser();

      wmId    = LOWORD(wParam);
      wmEvent = HIWORD(wParam);
      // Parse the menu selections:
      switch (wmId) {
      case IDM_EXIT:
        if (g_handler.get()) {
          g_handler->QuittingApp(true);
    	  g_handler->DispatchCloseToNextBrowser();
    	} else {
          DestroyWindow(hWnd);
		}
        return 0;
      case ID_WARN_CONSOLEMESSAGE:
/*
        if (g_handler.get()) {
          std::wstringstream ss;
          ss << L"Console messages will be written to "
              << std::wstring(CefString(g_handler->GetLogFile()));
          MessageBox(hWnd, ss.str().c_str(), L"Console Messages",
              MB_OK | MB_ICONINFORMATION);
        }
*/
        return 0;
      case ID_WARN_DOWNLOADCOMPLETE:
      case ID_WARN_DOWNLOADERROR:
        if (g_handler.get()) {
          std::wstringstream ss;
          ss << L"File \"" <<
              std::wstring(CefString(g_handler->GetLastDownloadFile())) <<
              L"\" ";

          if (wmId == ID_WARN_DOWNLOADCOMPLETE)
            ss << L"downloaded successfully.";
          else
            ss << L"failed to download.";

          MessageBox(hWnd, ss.str().c_str(), L"File Download",
              MB_OK | MB_ICONINFORMATION);
        }
        return 0;
#ifdef SHOW_TOOLBAR_UI
      case IDC_NAV_BACK:   // Back button
        if (browser.get())
          browser->GoBack();
        return 0;
      case IDC_NAV_FORWARD:  // Forward button
        if (browser.get())
          browser->GoForward();
        return 0;
      case IDC_NAV_RELOAD:  // Reload button
        if (browser.get())
          browser->Reload();
        return 0;
      case IDC_NAV_STOP:  // Stop button
        if (browser.get())
          browser->StopLoad();
        return 0;
#endif // SHOW_TOOLBAR_UI
      default:
          ExtensionString commandId = NativeMenuModel::getInstance(getMenuParent(g_handler->GetBrowser())).getCommandId(wmId);
          if (commandId.size() > 0) {
              CefRefPtr<CommandCallback> callback = new EditCommandCallback(g_handler->GetBrowser(), commandId);
              g_handler->SendJSCommand(g_handler->GetBrowser(), commandId, callback);
          }
      }
      break;
    }

    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
      return 0;

    case WM_SETFOCUS:
      if (g_handler.get() && g_handler->GetBrowser()) {
        // Pass focus to the browser window
        CefWindowHandle hwnd =
            g_handler->GetBrowser()->GetHost()->GetWindowHandle();
        if (hwnd)
          PostMessage(hwnd, WM_SETFOCUS, wParam, NULL);
      }
      return 0;

    case WM_SIZE:
      // Minimizing resizes the window to 0x0 which causes our layout to go all
      // screwy, so we just ignore it.
      if (wParam != SIZE_MINIMIZED && g_handler.get() &&
          g_handler->GetBrowser()) {
        CefWindowHandle hwnd =
            g_handler->GetBrowser()->GetHost()->GetWindowHandle();
        if (hwnd) {
          // Resize the browser window and address bar to match the new frame
          // window size
          RECT rect;
          GetClientRect(hWnd, &rect);
#ifdef SHOW_TOOLBAR_UI
          rect.top += URLBAR_HEIGHT;

          int urloffset = rect.left + BUTTON_WIDTH * 4;
#endif // SHOW_TOOLBAR_UI

          HDWP hdwp = BeginDeferWindowPos(1);
#ifdef SHOW_TOOLBAR_UI
          hdwp = DeferWindowPos(hdwp, editWnd, NULL, urloffset,
            0, rect.right - urloffset, URLBAR_HEIGHT, SWP_NOZORDER);
#endif // SHOW_TOOLBAR_UI
          hdwp = DeferWindowPos(hdwp, hwnd, NULL,
            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOZORDER);
          EndDeferWindowPos(hdwp);
        }
      }
      break;

    case WM_ERASEBKGND:
      if (g_handler.get() && g_handler->GetBrowser()) {
        CefWindowHandle hwnd =
            g_handler->GetBrowser()->GetHost()->GetWindowHandle();
        if (hwnd) {
          // Dont erase the background if the browser window has been loaded
          // (this avoids flashing)
          return 0;
        }
      }
      break;

    case WM_CLOSE:
      if (g_handler.get()) {

        HWND hWnd = GetActiveWindow();
        SaveWindowRect(hWnd);

        // If we already initiated the browser closing, then let default window proc handle it.
        HWND browserHwnd = g_handler->GetBrowser()->GetHost()->GetWindowHandle();
        HANDLE closing = GetProp(browserHwnd, CLOSING_PROP);
        if (closing) {
		    RemoveProp(browserHwnd, CLOSING_PROP);
			break;
		}

        g_handler->QuittingApp(true);
        g_handler->DispatchCloseToNextBrowser();
        return 0;
      }
      break;

    case WM_DESTROY:
      // The frame window has exited
      PostQuitMessage(0);
      return 0;

    case WM_DROPFILES:
        if (g_handler.get()) {
            return HandleDropFiles((HDROP)wParam, g_handler, g_handler->GetBrowser());
        }
        return 0;

    case WM_INITMENUPOPUP:
        HMENU menu = (HMENU)wParam;
        int count = GetMenuItemCount(menu);
        void* menuParent = getMenuParent(g_handler->GetBrowser());
        for (int i = 0; i < count; i++) {
            UINT id = GetMenuItemID(menu, i);

            bool enabled = NativeMenuModel::getInstance(menuParent).isMenuItemEnabled(id);
            UINT flagEnabled = enabled ? MF_ENABLED | MF_BYCOMMAND : MF_DISABLED | MF_BYCOMMAND;
            EnableMenuItem(menu, id,  flagEnabled);

            bool checked = NativeMenuModel::getInstance(menuParent).isMenuItemChecked(id);
            UINT flagChecked = checked ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
            CheckMenuItem(menu, id, flagChecked);
        }
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);
  switch (message) {
  case WM_INITDIALOG:
    return (INT_PTR)TRUE;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;
    }
    break;
  }
  return (INT_PTR)FALSE;
}


// Global functions

std::string AppGetWorkingDirectory() {
  return szWorkingDir;
}

CefString AppGetCachePath() {
  std::wstring cachePath = ClientApp::AppGetSupportDirectory();
  cachePath +=  L"/cef_data";

  return CefString(cachePath);
}

// Helper function for AppGetProductVersionString. Reads version info from
// VERSIONINFO and writes it into the passed in std::wstring.
void GetFileVersionString(std::wstring &retVersion) {
  DWORD dwSize = 0;
  BYTE *pVersionInfo = NULL;
  VS_FIXEDFILEINFO *pFileInfo = NULL;
  UINT pLenFileInfo = 0;

  HMODULE module = GetModuleHandle(NULL);
  TCHAR executablePath[MAX_PATH];
  GetModuleFileName(module, executablePath, MAX_PATH);

  dwSize = GetFileVersionInfoSize(executablePath, NULL);
  if (dwSize == 0) {
    return;
  }

  pVersionInfo = new BYTE[dwSize];

  if (!GetFileVersionInfo(executablePath, 0, dwSize, pVersionInfo)) 	{
    delete[] pVersionInfo;
    return;
  }

  if (!VerQueryValue(pVersionInfo, TEXT("\\"), (LPVOID*) &pFileInfo, &pLenFileInfo)) {
    delete[] pVersionInfo;
    return;
  }

  int major  = (pFileInfo->dwFileVersionMS >> 16) & 0xffff ;
  int minor  = (pFileInfo->dwFileVersionMS) & 0xffff;
  int hotfix = (pFileInfo->dwFileVersionLS >> 16) & 0xffff;
  int other  = (pFileInfo->dwFileVersionLS) & 0xffff;

  delete[] pVersionInfo;

  std::wostringstream versionStream(L"");
  versionStream << major << L"." << minor << L"." << hotfix << L"." << other; 
  retVersion = versionStream.str();
}

CefString AppGetProductVersionString() {
  std::wstring s(APP_NAME);
  size_t i = s.find(L" ");
  while (i != std::wstring::npos) {
    s.erase(i, 1);
    i = s.find(L" ");
  }
  std::wstring version(L"");
  GetFileVersionString(version);
  s.append(L"/");
  s.append(version);
  return CefString(s);
}
