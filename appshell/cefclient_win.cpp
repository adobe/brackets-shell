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

// Global Variables:
HINSTANCE       gInstance;
DWORD           gAppStartupTime;
HACCEL          gAccelTable;
std::wstring    gFilesToOpen;           // Filenames passed as arguments to app

// Static Variables
static char     gWorkingDir[MAX_PATH] = {0};
static TCHAR    gInitialUrl[MAX_PATH] = {0};

// Externals
extern CefRefPtr<ClientHandler> gHandler;


// Forward declarations of functions included in this code module:
BOOL InitInstance(HINSTANCE, int);

// The global ClientHandler reference.

#if defined(OS_WIN)
// Add Common Controls to the application manifest because it's required to
// support the default tooltip implementation.
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")  // NOLINT(whitespace/line_length)
#endif

// If 'str' ends with a colon followed by some digits, then remove the colon and digits. For example:
//    "c:\bob\abc.txt:123:456" will be changed to "c:\bob\abc.txt:123"
//    "c:\bob\abc.txt:123" will be changed to "c:\bob\abc.txt"
//    "c:\bob\abc.txt" will not be changed
// (Note: we could do this with a regular expression, but there is no regex library currently
// built into brackets-shell, and I don't want to add one just for this simple case).
void StripColonNumber(std::wstring& str) {
    bool gotDigits = false;
    int index;
    for (index = str.size() - 1; index >= 0; index--) {
        if (!isdigit(str[index]))
            break;
        gotDigits = true;
    }
    if (gotDigits && index >= 0 && str[index] == ':') {
        str.resize(index);
    }
}

// Determine if 'str' is a valid filename.
bool IsFilename(const std::wstring& str) {
    // Strip off trailing line and column number, if any.
    std::wstring temp(str);
    StripColonNumber(temp);
    StripColonNumber(temp);

	// Return true if the OS thinks the filename is OK.
    return (GetFileAttributes(temp.c_str()) != INVALID_FILE_ATTRIBUTES);
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

    gAppStartupTime = timeGetTime();

    CefMainArgs main_args(hInstance);
    CefRefPtr<ClientApp> app(new ClientApp);

    // Execute the secondary process, if any.
    int exit_code = CefExecuteProcess(main_args, app.get());
    if (exit_code >= 0)
        return exit_code;

    // Retrieve the current working directory.
    if (_getcwd(gWorkingDir, MAX_PATH) == NULL)
        gWorkingDir[0] = 0;

    // Parse command line arguments. The passed in values are ignored on Windows.
    AppInitCommandLine(0, NULL);

    CefSettings settings;

    // Populate the settings based on command line arguments.
    AppGetSettings(settings, app);

    // Check command
    if (CefString(&settings.cache_path).length() == 0)
	    CefString(&settings.cache_path) = AppGetCachePath();

    // Initialize CEF.
    CefInitialize(main_args, settings, app.get());

    CefRefPtr<CefCommandLine> cmdLine = AppGetCommandLine();
    if (cmdLine->HasSwitch(cefclient::kStartupPath)) 
    {
	    wcscpy(gInitialUrl, cmdLine->GetSwitchValue(cefclient::kStartupPath).c_str());
    }
    else 
    {
        // If the shift key is not pressed, look for the index.html file 
        if (GetAsyncKeyState(VK_SHIFT) == 0) 
        {
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
            if (GetFileAttributes(appPath) != INVALID_FILE_ATTRIBUTES) 
	            wcscpy(gInitialUrl, appPath);

            if (!wcslen(gInitialUrl)) 
            {
	            // Look for .\www\index.html next
	            wcscpy(pathRoot, L"\\www\\index.html");
	            if (GetFileAttributes(appPath) != INVALID_FILE_ATTRIBUTES)
	                wcscpy(gInitialUrl, appPath);
            }
        }
    }

    if (!wcslen(gInitialUrl)) 
    {
        // If we got here, either the startup file couldn't be found, or the user pressed the
        // shift key while launching. Prompt to select the index.html file.
        OPENFILENAME ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = gInitialUrl;
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

    if (!settings.multi_threaded_message_loop) 
    {
        // Run the CEF message loop. This function will block until the application
        // recieves a WM_QUIT message.
        CefRunMessageLoop();
    } 
    else 
    {
        MSG msg;

        // Run the application message loop.
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (!TranslateAccelerator(msg.hwnd, gAccelTable, &msg)) {
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
      if (wParam == VK_RETURN && gHandler.get()) {
        // When the user hits the enter key load the URL
        CefRefPtr<CefBrowser> browser = gHandler->GetBrowser();
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
      gHandler = new ClientHandler();
      gHandler->SetMainHwnd(hWnd);

      // Create the child windows used for navigation
      RECT rect;
      int x = 0;

      GetClientRect(hWnd, &rect);

      CefWindowInfo info;
      CefBrowserSettings settings;

      settings.web_security = STATE_DISABLED;

      // Initialize window info to the defaults for a child window
      info.SetAsChild(hWnd, rect);

      // Creat the new child browser window
      CefBrowserHost::CreateBrowser(info,
          static_cast<CefRefPtr<CefClient> >(gHandler),
          gInitialUrl, settings);

      return 0;
    }

    case WM_COMMAND: {
      CefRefPtr<CefBrowser> browser;
      if (gHandler.get())
        browser = gHandler->GetBrowser();

      wmId    = LOWORD(wParam);
      wmEvent = HIWORD(wParam);
      // Parse the menu selections:
      switch (wmId) {
      case IDM_EXIT:
        if (gHandler.get()) {
          gHandler->QuittingApp(true);
    	  gHandler->DispatchCloseToNextBrowser();
    	} else {
          DestroyWindow(hWnd);
		}
        return 0;
      case ID_WARN_CONSOLEMESSAGE:
        return 0;
      case ID_WARN_DOWNLOADCOMPLETE:
      case ID_WARN_DOWNLOADERROR:
        if (gHandler.get()) {
          std::wstringstream ss;
          ss << L"File \"" <<
              std::wstring(CefString(gHandler->GetLastDownloadFile())) <<
              L"\" ";

          if (wmId == ID_WARN_DOWNLOADCOMPLETE)
            ss << L"downloaded successfully.";
          else
            ss << L"failed to download.";

          MessageBox(hWnd, ss.str().c_str(), L"File Download",
              MB_OK | MB_ICONINFORMATION);
        }
        return 0;

      default:
          ExtensionString commandId = NativeMenuModel::getInstance(getMenuParent(gHandler->GetBrowser())).getCommandId(wmId);
          if (commandId.size() > 0) {
              CefRefPtr<CommandCallback> callback = new EditCommandCallback(gHandler->GetBrowser(), commandId);
              gHandler->SendJSCommand(gHandler->GetBrowser(), commandId, callback);
          }
      }
      break;
    }

    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
      return 0;

    case WM_SETFOCUS:
      if (gHandler.get() && gHandler->GetBrowser()) {
        // Pass focus to the browser window
        CefWindowHandle hwnd =
            gHandler->GetBrowser()->GetHost()->GetWindowHandle();
        if (hwnd)
          PostMessage(hwnd, WM_SETFOCUS, wParam, NULL);
      }
      return 0;

    case WM_SIZE:
      // Minimizing resizes the window to 0x0 which causes our layout to go all
      // screwy, so we just ignore it.
      if (wParam != SIZE_MINIMIZED && gHandler.get() &&
          gHandler->GetBrowser()) {
        CefWindowHandle hwnd =
            gHandler->GetBrowser()->GetHost()->GetWindowHandle();
        if (hwnd) {
          // Resize the browser window and address bar to match the new frame
          // window size
          RECT rect;
          GetClientRect(hWnd, &rect);


          HDWP hdwp = BeginDeferWindowPos(1);

          hdwp = DeferWindowPos(hdwp, hwnd, NULL,
            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOZORDER);
          EndDeferWindowPos(hdwp);
        }
      }
      break;

    case WM_ERASEBKGND:
      if (gHandler.get() && gHandler->GetBrowser()) {
        CefWindowHandle hwnd =
            gHandler->GetBrowser()->GetHost()->GetWindowHandle();
        if (hwnd) {
          // Dont erase the background if the browser window has been loaded
          // (this avoids flashing)
          return 0;
        }
      }
      break;

    case WM_CLOSE:
      if (gHandler.get()) {

        HWND hWnd = GetActiveWindow();
        SaveWindowRect(hWnd);

        // If we already initiated the browser closing, then let default window proc handle it.
        HWND browserHwnd = gHandler->GetBrowser()->GetHost()->GetWindowHandle();
        HANDLE closing = GetProp(browserHwnd, CLOSING_PROP);
        if (closing) {
		    RemoveProp(browserHwnd, CLOSING_PROP);
			break;
		}

        gHandler->QuittingApp(true);
        gHandler->DispatchCloseToNextBrowser();
        return 0;
      }
      break;

    case WM_DESTROY:
      // The frame window has exited
      PostQuitMessage(0);
      return 0;

    case WM_DROPFILES:
        if (gHandler.get()) {
            return HandleDropFiles((HDROP)wParam, gHandler, gHandler->GetBrowser());
        }
        return 0;

    case WM_INITMENUPOPUP:
        // Notify before popping up
        gHandler->SendJSCommand(gHandler->GetBrowser(), APP_BEFORE_MENUPOPUP);

        HMENU menu = (HMENU)wParam;
        int count = GetMenuItemCount(menu);
        void* menuParent = getMenuParent(gHandler->GetBrowser());
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

// Global functions

std::string AppGetWorkingDirectory() {
  return gWorkingDir;
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
