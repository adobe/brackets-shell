// Copyright (c) 2010 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "config.h"
#include "cefclient.h"
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <direct.h>
#include <MMSystem.h>
#include <sstream>
#include <string>
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "client_handler.h"
#include "resource.h"
#include "string_util.h"

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
TCHAR szTitle[MAX_LOADSTRING];  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];  // the main window class name
char szWorkingDir[MAX_PATH];  // The current working directory

TCHAR szInitialUrl[MAX_PATH] = {0};

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
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

  HACCEL hAccelTable;

  // Initialize global strings
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadString(hInstance, IDC_CEFCLIENT, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);
  
  HKEY hKey;
  DWORD lResult;
  #define PREF_NAME L"Software\\Brackets\\InitialURL"

  // If the Control key is down, delete the prefs
  BOOL bDeletePrefs = false;
  if (GetAsyncKeyState(VK_CONTROL) != 0) {
    bDeletePrefs = true;
  }

  // Don't read the prefs if the shift key is down
  if (GetAsyncKeyState(VK_SHIFT) == 0) {
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, PREF_NAME, 0, KEY_READ, &hKey)) {
      if (bDeletePrefs) {
        RegDeleteKeyEx(hKey, L"", KEY_WOW64_32KEY, 0);
      } else {
        DWORD length = MAX_PATH;
        RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)szInitialUrl, &length);
        RegCloseKey(hKey);
      }
    }

    if (!wcslen(szInitialUrl)) {
      // Look for www/index.html in the same directory as the application
      wchar_t appPath[MAX_PATH];
      GetModuleFileName(NULL, appPath, MAX_PATH);

      wcscpy(wcsrchr(appPath, '\\'), L"\\www\\index.html");

      // If the file exists, use it
      if (GetFileAttributes(appPath) != INVALID_FILE_ATTRIBUTES) {
        wcscpy(szInitialUrl, appPath);
      }
    }
  }

  if (!wcslen(szInitialUrl)) {
      OPENFILENAME ofn = {0};
      ofn.lStructSize = sizeof(ofn);
      ofn.lpstrFile = szInitialUrl;
      ofn.nMaxFile = MAX_PATH;
      ofn.lpstrFilter = L"Web Files\0*.htm;*.html\0\0";
      ofn.lpstrTitle = L"Please select the brackets index.html file.";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

      if (GetOpenFileName(&ofn)) {
        lResult = RegCreateKeyEx(HKEY_CURRENT_USER, PREF_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
        if (lResult == ERROR_SUCCESS) {
          RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE)szInitialUrl, (wcslen(szInitialUrl) + 1) * 2);
          RegCloseKey(hKey);
        }
      } else {
        // User cancelled, exit the app
        CefShutdown();
        return 0;
      }
  }

  // Perform application initialization
  if (!InitInstance (hInstance, nCmdShow))
    return FALSE;

  hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CEFCLIENT));

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
ATOM MyRegisterClass(HINSTANCE hInstance) {
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
  wcex.lpszMenuName  = MAKEINTRESOURCE(IDC_CEFCLIENT);
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
  HWND hWnd;

  hInst = hInstance;  // Store instance handle in our global variable

  hWnd = CreateWindow(szWindowClass, szTitle,
                      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, 0,
                      CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

  if (!hWnd)
    return FALSE;

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
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
      case IDM_ABOUT:
        if (browser) {
            g_handler->SendJSCommand(browser, HELP_ABOUT);
        }
        return 0;
      case IDM_EXIT:
        if (g_handler.get()) {
          g_handler->QuittingApp(true);
    	  g_handler->DispatchCloseToNextBrowser();
    	} else {
          DestroyWindow(hWnd);
		}
        return 0;
      case ID_WARN_CONSOLEMESSAGE:
        if (g_handler.get()) {
          std::wstringstream ss;
          ss << L"Console messages will be written to "
              << std::wstring(CefString(g_handler->GetLogFile()));
          MessageBox(hWnd, ss.str().c_str(), L"Console Messages",
              MB_OK | MB_ICONINFORMATION);
        }
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
  // Brackets: Set persistance cache
  wchar_t dataPath[MAX_PATH];
  SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, dataPath);
  
  std::wstring cachePath = dataPath;
  cachePath += L"\\Brackets\\cef_data";

  return CefString(cachePath);
}