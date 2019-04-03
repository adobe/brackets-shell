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
#include "include/cef_version.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "client_handler.h"
#include "config.h"
#include "appshell/browser/resource.h"
#include "appshell/common/client_switches.h"
#include "appshell/appshell_helpers.h"
#include "native_menu_model.h"
#include "appshell_node_process.h"

#include <algorithm>
#include <ShellAPI.h>
#include <ShlObj.h>

#include "cef_registry.h"
#include "cef_main_window.h"
#include "update.h"

// Global Variables:
DWORD            g_appStartupTime;
HINSTANCE        hInst;                     // current instance
HACCEL           hAccelTable;
std::wstring     gFilesToOpen;              // Filenames passed as arguments to app
cef_main_window* gMainWnd = NULL;

// static variables (not exported)
static char      szWorkingDir[MAX_UNC_PATH];    // The current working directory
static wchar_t   szInitialUrl[MAX_UNC_PATH] = {0};


// Forward declarations of functions included in this code module:
BOOL InitInstance(HINSTANCE, int);

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

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

bool GetFullPath(const std::wstring& path, std::wstring& oFullPath)
{

  DWORD retval;
  TCHAR  buffer[MAX_UNC_PATH] = TEXT(""); 
  TCHAR  buf[MAX_UNC_PATH]    = TEXT(""); 

  if(path.length() <= 0) {
    return false;
  }

  if(path[0] == L'"' && path[path.length() -1] == L'"') {
    // This is a special case where arguments are sent
    // as quotes(e.g.:file names having spaces). In this
    // case we first strip the quotes, get the path 
    // and finally append the quotes to the result.
    std::wstring normalizedPath = path;
    normalizedPath.erase (std::remove(normalizedPath.begin(), normalizedPath.end(), L'"'), normalizedPath.end());

    retval = GetFullPathName( normalizedPath.c_str(),
                              MAX_UNC_PATH,
                              buffer,
                              NULL );
    // Now add the quotes to the final string.
    if(retval) {
       oFullPath = L'"';
       oFullPath = oFullPath + buffer + L'"';
    }

  }
  else {
  
    // Retrieve the full path name for a file. 
    // The file does not need to exist.
    retval = GetFullPathName( path.c_str(),
                              MAX_UNC_PATH,
                              buffer,
                              NULL );
    if(retval)
      oFullPath = buffer;
  }

  return (retval == 0) ? false : true;

}

std::wstring GetFilenamesFromCommandLine(CefRefPtr<CefCommandLine> command_line) {
  std::wstring result = L"[]";

  if (command_line->HasArguments()) {
    bool firstEntry = true;
    std::vector<CefString> args;
    command_line->GetArguments(args);
    std::vector<CefString>::iterator iterator;
    result = L"[";
    for (iterator = args.begin(); iterator != args.end(); iterator++) {
      std::wstring argument = (*iterator).ToWString();
      if (IsFilename(argument)) {
        std::wstring fullPath;
        // We check if this is a valid file path. If not just ignore this parameter.
        if( !GetFullPath(argument, fullPath) )
          continue;
  
        if (!firstEntry) {
          result += L",";
        }
  
        firstEntry = false;
  
        result += L"\"" + fullPath + L"\"";
      }
    }
    result += L"]";
  }

  return result;
}

// forward declaration; implemented in appshell_extensions_win.cpp
void ConvertToUnixPath(ExtensionString& filename);

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
  int exit_code = CefExecuteProcess(main_args, app.get(), NULL);
  if (exit_code >= 0)
    return exit_code;

  bool isShiftKeyDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? true: false;

  // Retrieve the current working directory.
  if (_getcwd(szWorkingDir, MAX_UNC_PATH) == NULL)
    szWorkingDir[0] = 0;

  // Parse command line arguments.
  CefRefPtr<CefCommandLine> cmdLine = CefCommandLine::CreateCommandLine();
  cmdLine->InitFromString(::GetCommandLineW());

  // Determine if we should use an already running instance of Brackets.
  HANDLE hMutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE, FIRST_INSTANCE_MUTEX_NAME);
  if ((hMutex != NULL) && cmdLine->HasArguments() && (lpCmdLine != NULL)) {
   // for subsequent instances, re-use an already running instance if we're being called to
   //   open an existing file on the command-line (eg. Open With.. from Windows Explorer)
   HWND hFirstInstanceWnd = cef_main_window::FindFirstTopLevelInstance();
   if (hFirstInstanceWnd != NULL) {
     ::SetForegroundWindow(hFirstInstanceWnd);
    if (::IsIconic(hFirstInstanceWnd))
      ::ShowWindow(hFirstInstanceWnd, SW_RESTORE);

      // message the other Brackets instance to actually open the given filename
      std::wstring filename = lpCmdLine;
      std::wstring wstrFilename;
      // We check if this is a valid file path. If not just ignore this parameter.
      if (GetFullPath(filename, wstrFilename)) {
        ConvertToUnixPath(wstrFilename);
        // note: WM_COPYDATA will manage passing the string across process space
        COPYDATASTRUCT data;
        data.dwData = ID_WM_COPYDATA_SENDOPENFILECOMMAND;
        data.cbData = (wstrFilename.length() + 1) * sizeof(WCHAR);
        data.lpData = (LPVOID)wstrFilename.c_str();
        ::SendMessage(hFirstInstanceWnd, WM_COPYDATA, (WPARAM)(HWND)hFirstInstanceWnd, (LPARAM)(LPVOID)&data);
        // exit this instance
        return 0;
      }
    }
    // otherwise, fall thru and launch a new instance
  }

  if (hMutex == NULL) {
	  // first instance of this app, so create the mutex and continue execution of this instance.
	  hMutex = ::CreateMutex(NULL, FALSE, FIRST_INSTANCE_MUTEX_NAME);
  }

  CefSettings settings;

  // Populate the settings based on command line arguments.
  AppGetSettings(settings, cmdLine);

  // Check command
  if (CefString(&settings.cache_path).length() == 0) {
	  CefString(&settings.cache_path) = appshell::AppGetCachePath();
  }

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get(), NULL);

  if (cmdLine->HasSwitch(client::switches::kStartupPath)) {
	  wcscpy(szInitialUrl, cmdLine->GetSwitchValue(client::switches::kStartupPath).c_str());
  }
  else {
	// If the shift key is not pressed, look for the index.html file 
	if (!isShiftKeyDown) {
	// Get the full pathname for the app. We look for the index.html
	// file relative to this location.
	wchar_t appPath[MAX_UNC_PATH];
	wchar_t *pathRoot;
	GetModuleFileName(NULL, appPath, MAX_UNC_PATH);

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
      ofn.nMaxFile = MAX_UNC_PATH;
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

  gFilesToOpen = GetFilenamesFromCommandLine(cmdLine);

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

  // Run the app auto update
  RunAppUpdate();

  // release the first instance mutex
  if (hMutex != NULL)
	  ReleaseMutex(hMutex);

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
  hInst = hInstance;  // Store instance handle in our global variable
  gMainWnd = new cef_main_window();
  return gMainWnd->Create();
}

// Global functions

std::string AppGetWorkingDirectory() {
  return szWorkingDir;
}

CefString AppGetInitialURL() {
    return szInitialUrl;    
}
