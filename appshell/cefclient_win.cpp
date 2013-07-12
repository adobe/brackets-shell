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

#include "cef_main_window.h"

// Global Variables:
HINSTANCE           gInstance;
DWORD               gAppStartupTime;
HACCEL              gAccelTable;
std::wstring        gFilesToOpen;           // Filenames passed as arguments to app
cef_main_window*    gMainWnd;

// Static Variables
static char         gWorkingDir[MAX_PATH] = {0};
static wchar_t      gInitialUrl[MAX_PATH] = {0};

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
static void StripColonNumber(std::wstring& str) 
{
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
static bool IsFilename(const std::wstring& str) 
{
    // Strip off trailing line and column number, if any.
    std::wstring temp(str);
    StripColonNumber(temp);
    StripColonNumber(temp);

	// Return true if the OS thinks the filename is OK.
    return (GetFileAttributes(temp.c_str()) != INVALID_FILE_ATTRIBUTES);
}

static std::wstring GetFilenamesFromCommandLine() 
{
    std::wstring result = L"[]";

    if (AppGetCommandLine()->HasArguments()) 
    {
        bool firstEntry = true;
        std::vector<CefString> args;
        AppGetCommandLine()->GetArguments(args);
        std::vector<CefString>::iterator iterator;

        result = L"[";
        for (iterator = args.begin(); iterator != args.end(); iterator++) 
        {
            std::wstring argument = (*iterator).ToWString();
            if (IsFilename(argument)) 
            {
                if (!firstEntry) 
                {
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
                     int       nCmdShow) 
{
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

	gInstance = hInstance;  // Store instance handle in our global variable
	gMainWnd = new cef_main_window();
	return gMainWnd->Create();
}

// Global functions

std::string AppGetWorkingDirectory() {
    return gWorkingDir;
}

CefString AppGetInitialURL() {
    return gInitialUrl;    
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
