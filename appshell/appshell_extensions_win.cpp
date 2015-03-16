/*
 * Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
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
 * 
 */ 

#include "appshell_extensions.h"
#include "native_menu_model.h"
#include "client_handler.h"

#include <algorithm>
#include <CommDlg.h>
#include <Psapi.h>
#include <ShellAPI.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <Shobjidl.h>
#include <stdio.h>
#include <sys/stat.h>
#include "config.h"
#define CLOSING_PROP L"CLOSING"
#define UNICODE_MINUS 0x2212
#define UNICODE_LEFT_ARROW 0x2190
#define UNICODE_DOWN_ARROW 0x2193

// Forward declarations for functions at the bottom of this file
void ConvertToNativePath(ExtensionString& filename);
void ConvertToUnixPath(ExtensionString& filename);
void RemoveTrailingSlash(ExtensionString& filename);
int ConvertErrnoCode(int errorCode, bool isReading = true);
int ConvertWinErrorCode(int errorCode, bool isReading = true);
static std::wstring GetPathToLiveBrowser();
static bool ConvertToShortPathName(std::wstring & path);
time_t FiletimeToTime(FILETIME const& ft);

// Redraw timeout variables. See the comment above ScheduleMenuRedraw for details.
const DWORD kMenuRedrawTimeout = 100;
UINT_PTR redrawTimerId = NULL;
CefRefPtr<CefBrowser> redrawBrowser;

extern HINSTANCE hInst;
extern HACCEL hAccelTable;
extern std::wstring gFilesToOpen;

// constants
#define MAX_LOADSTRING 100

///////////////////////////////////////////////////////////////////////////////
// LiveBrowserMgrWin

class LiveBrowserMgrWin
{
public:
    static LiveBrowserMgrWin* GetInstance();
    static void Shutdown();

    bool IsChromeWindow(HWND hwnd);
    bool IsAnyChromeWindowsRunning();
    void CloseLiveBrowserKillTimers();
    void CloseLiveBrowserFireCallback(int valToSend);

    static BOOL CALLBACK EnumChromeWindowsCallback(HWND hwnd, LPARAM userParam);
    static void CALLBACK CloseLiveBrowserTimerCallback( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
    static void CALLBACK CloseLiveBrowserAsyncCallback( HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult );

    CefRefPtr<CefProcessMessage> GetCloseCallback() { return m_closeLiveBrowserCallback; }
    UINT GetCloseHeartbeatTimerId() { return m_closeLiveBrowserHeartbeatTimerId; }
    UINT GetCloseTimeoutTimerId() { return m_closeLiveBrowserTimeoutTimerId; }

    void SetCloseCallback(CefRefPtr<CefProcessMessage> closeLiveBrowserCallback)
        { m_closeLiveBrowserCallback = closeLiveBrowserCallback; }
    void SetBrowser(CefRefPtr<CefBrowser> browser)
        { m_browser = browser; }
    void SetCloseHeartbeatTimerId(UINT closeLiveBrowserHeartbeatTimerId)
        { m_closeLiveBrowserHeartbeatTimerId = closeLiveBrowserHeartbeatTimerId; }
    void SetCloseTimeoutTimerId(UINT closeLiveBrowserTimeoutTimerId)
        { m_closeLiveBrowserTimeoutTimerId = closeLiveBrowserTimeoutTimerId; }

private:
    // private so this class cannot be instantiated externally
    LiveBrowserMgrWin();
    virtual ~LiveBrowserMgrWin();

    UINT                            m_closeLiveBrowserHeartbeatTimerId;
    UINT                            m_closeLiveBrowserTimeoutTimerId;
    CefRefPtr<CefProcessMessage>    m_closeLiveBrowserCallback;
    CefRefPtr<CefBrowser>            m_browser;

    static LiveBrowserMgrWin* s_instance;
};

LiveBrowserMgrWin::LiveBrowserMgrWin()
    : m_closeLiveBrowserHeartbeatTimerId(0)
    , m_closeLiveBrowserTimeoutTimerId(0)
{
}

LiveBrowserMgrWin::~LiveBrowserMgrWin()
{
}

LiveBrowserMgrWin* LiveBrowserMgrWin::GetInstance()
{
    if (!s_instance)
        s_instance = new LiveBrowserMgrWin();
    return s_instance;
}

void LiveBrowserMgrWin::Shutdown()
{
    delete s_instance;
    s_instance = NULL;
}

bool LiveBrowserMgrWin::IsChromeWindow(HWND hwnd)
{
    if( !hwnd ) {
        return false;
    }

    //Find the path that opened this window
    DWORD processId = 0;
    ::GetWindowThreadProcessId(hwnd, &processId);

    HANDLE processHandle = ::OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if( !processHandle ) { 
        return false;
    }

    DWORD modulePathBufSize = MAX_UNC_PATH+1;
    WCHAR modulePathBuf[MAX_UNC_PATH+1];
    DWORD modulePathSize = ::GetModuleFileNameEx(processHandle, NULL, modulePathBuf, modulePathBufSize );
    ::CloseHandle(processHandle);
    processHandle = NULL;

    std::wstring modulePath(modulePathBuf, modulePathSize);

    //See if this path is the same as what we want to launch
    std::wstring appPath = GetPathToLiveBrowser();

    if( !ConvertToShortPathName(modulePath) || !ConvertToShortPathName(appPath) ) {
        return false;
    }

    if(0 != _wcsicmp(appPath.c_str(), modulePath.c_str()) ){
        return false;
    }

    //looks good
    return true;
}

struct EnumChromeWindowsCallbackData
{
    bool    closeWindow;
    int     numberOfFoundWindows;
};

BOOL CALLBACK LiveBrowserMgrWin::EnumChromeWindowsCallback(HWND hwnd, LPARAM userParam)
{
    if( !hwnd || !s_instance) {
        return FALSE;
    }

    EnumChromeWindowsCallbackData* cbData = reinterpret_cast<EnumChromeWindowsCallbackData*>(userParam);
    if(!cbData) {
        return FALSE;
    }

    if (!s_instance->IsChromeWindow(hwnd)) {
        return TRUE;
    }

    cbData->numberOfFoundWindows++;
    //This window belongs to the instance of the browser we're interested in, tell it to close
    if( cbData->closeWindow ) {
        ::SendMessageCallback(hwnd, WM_CLOSE, NULL, NULL, CloseLiveBrowserAsyncCallback, NULL);
    }

    return TRUE;
}

bool LiveBrowserMgrWin::IsAnyChromeWindowsRunning()
{
    EnumChromeWindowsCallbackData cbData = {0};
    cbData.numberOfFoundWindows = 0;
    cbData.closeWindow = false;
    ::EnumWindows(EnumChromeWindowsCallback, (LPARAM)&cbData);
    return( cbData.numberOfFoundWindows != 0 );
}

void LiveBrowserMgrWin::CloseLiveBrowserKillTimers()
{
    if (m_closeLiveBrowserHeartbeatTimerId) {
        ::KillTimer(NULL, m_closeLiveBrowserHeartbeatTimerId);
        m_closeLiveBrowserHeartbeatTimerId = 0;
    }

    if (m_closeLiveBrowserTimeoutTimerId) {
        ::KillTimer(NULL, m_closeLiveBrowserTimeoutTimerId);
        m_closeLiveBrowserTimeoutTimerId = 0;
    }
}

void LiveBrowserMgrWin::CloseLiveBrowserFireCallback(int valToSend)
{
    CefRefPtr<CefListValue> responseArgs = m_closeLiveBrowserCallback->GetArgumentList();
    
    // kill the timers
    CloseLiveBrowserKillTimers();
    
    // Set common response args (callbackId and error)
    responseArgs->SetInt(1, valToSend);
    
    // Send response
    m_browser->SendProcessMessage(PID_RENDERER, m_closeLiveBrowserCallback);
    
    // Clear state
    m_closeLiveBrowserCallback = NULL;
    m_browser = NULL;
}

void CALLBACK LiveBrowserMgrWin::CloseLiveBrowserTimerCallback( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    if( !s_instance ) {
        ::KillTimer(NULL, idEvent);
        return;
    }

    int retVal =  NO_ERROR;
    if( s_instance->IsAnyChromeWindowsRunning() )
    {
        retVal = ERR_UNKNOWN;
        //if this is the heartbeat timer, wait for another beat
        if (idEvent == s_instance->m_closeLiveBrowserHeartbeatTimerId) {
            return;
        }
    }

    //notify back to the app
    s_instance->CloseLiveBrowserFireCallback(retVal);
}

void CALLBACK LiveBrowserMgrWin::CloseLiveBrowserAsyncCallback( HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult )
{
    if( !s_instance ) {
        return;
    }

    //If there are no more versions of chrome, then fire the callback
    if( !s_instance->IsAnyChromeWindowsRunning() ) {
        s_instance->CloseLiveBrowserFireCallback(NO_ERROR);
    }
    else if(s_instance->m_closeLiveBrowserHeartbeatTimerId == 0){
        //start a heartbeat timer to see if it closes after the message returned
        s_instance->m_closeLiveBrowserHeartbeatTimerId = ::SetTimer(NULL, 0, 30, CloseLiveBrowserTimerCallback);
    }
}

LiveBrowserMgrWin* LiveBrowserMgrWin::s_instance = NULL;


static int CALLBACK SetInitialPathCallback(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (BFFM_INITIALIZED == uMsg && NULL != lpData)
    {
        SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
    }

    return 0;
}

static std::wstring GetPathToLiveBrowser() 
{
    HKEY hKey;

    // First, look at the "App Paths" registry key for a "chrome.exe" entry. This only
    // checks for installs for all users. If Chrome is only installed for the current user,
    // we fall back to the code below.
    if (ERROR_SUCCESS == RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, 
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe",
            0, KEY_READ, &hKey)) {
       wchar_t wpath[MAX_UNC_PATH] = {0};

        DWORD length = MAX_UNC_PATH;
        RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)wpath, &length);
        RegCloseKey(hKey);

        return std::wstring(wpath);
    }

    // We didn't get an "App Paths" entry. This could be because Chrome was only installed for
    // the current user, or because Chrome isn't installed at all.
    // Look for Chrome.exe at C:\Users\{USERNAME}\AppData\Local\Google\Chrome\Application\chrome.exe
    TCHAR localAppPath[MAX_UNC_PATH] = {0};
    SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, localAppPath);
    std::wstring appPath(localAppPath);
    appPath += L"\\Google\\Chrome\\Application\\chrome.exe";
        
    return appPath;
}
    
static bool ConvertToShortPathName(std::wstring & path)
{
    DWORD shortPathBufSize = MAX_UNC_PATH+1;
    WCHAR shortPathBuf[MAX_UNC_PATH+1];
    DWORD finalShortPathSize = ::GetShortPathName(path.c_str(), shortPathBuf, shortPathBufSize);
    if( finalShortPathSize == 0 ) {
        return false;
    }
        
    path.assign(shortPathBuf, finalShortPathSize);
    return true;
}

int32 OpenLiveBrowser(ExtensionString argURL, bool enableRemoteDebugging)
{
    std::wstring appPath = GetPathToLiveBrowser();
    std::wstring args = appPath;

    if (enableRemoteDebugging) {
        std::wstring profilePath(ClientApp::AppGetSupportDirectory());
        profilePath += L"\\live-dev-profile";
        args += L" --user-data-dir=\"";
        args += profilePath;
        args += L"\" --no-first-run --no-default-browser-check --allow-file-access-from-files --remote-debugging-port=9222 ";
    } else {
        args += L" ";
    }
    args += argURL;

    // Args must be mutable
    int argsBufSize = args.length() +1;
    std::vector<WCHAR> argsBuf;
    argsBuf.resize(argsBufSize);
    wcscpy(&argsBuf[0], args.c_str());

    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    // Launch cmd.exe and pass in the arguments
    if (!CreateProcess(NULL, &argsBuf[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return ConvertWinErrorCode(GetLastError());
    }
        
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return NO_ERROR;
}

void CloseLiveBrowser(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    LiveBrowserMgrWin* liveBrowserMgr = LiveBrowserMgrWin::GetInstance();
    
    if (liveBrowserMgr->GetCloseCallback() != NULL) {
        // We can only handle a single async callback at a time. If there is already one that hasn't fired then
        // we kill it now and get ready for the next.
        liveBrowserMgr->CloseLiveBrowserFireCallback(ERR_UNKNOWN);
    }

    liveBrowserMgr->SetCloseCallback(response);
    liveBrowserMgr->SetBrowser(browser);

    EnumChromeWindowsCallbackData cbData = {0};

    cbData.numberOfFoundWindows = 0;
    cbData.closeWindow = true;
    ::EnumWindows(LiveBrowserMgrWin::EnumChromeWindowsCallback, (LPARAM)&cbData);

    if (cbData.numberOfFoundWindows == 0) {
        liveBrowserMgr->CloseLiveBrowserFireCallback(NO_ERROR);
    } else if (liveBrowserMgr->GetCloseCallback()) {
        // set a timeout for up to 10 seconds to close the browser
        liveBrowserMgr->SetCloseTimeoutTimerId( ::SetTimer(NULL, 0, 10 * 1000, LiveBrowserMgrWin::CloseLiveBrowserTimerCallback) );
    }
}

int32 OpenURLInDefaultBrowser(ExtensionString url)
{
    DWORD result = (DWORD)ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

    // If the result > 32, the function suceeded. If the result is <= 32, it is an
    // error code.
    if (result <= 32)
        return ConvertWinErrorCode(result);

    return NO_ERROR;
}

int32 ShowOpenDialog(bool allowMultipleSelection,
                     bool chooseDirectory,
                     ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString fileTypes,
                     CefRefPtr<CefListValue>& selectedFiles)
{
    wchar_t szFile[MAX_UNC_PATH];
    szFile[0] = 0;

    // Windows common file dialogs can handle Windows path only, not Unix path.
    // ofn.lpstrInitialDir also needs Windows path on XP and not Unix path.
    ConvertToNativePath(initialDirectory);

    if (chooseDirectory) {
        // check current OS version
        OSVERSIONINFO osvi;
        memset(&osvi, 0, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&osvi) && (osvi.dwMajorVersion >= 6)) {
            // for Vista or later, use the MSDN-preferred implementation of the Open File dialog in pick folders mode
            IFileDialog *pfd;
            if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
                // configure the dialog to Select Folders only
                DWORD dwOptions;
                if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
                    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_DONTADDTORECENT);
                    IShellItem *shellItem = NULL;
                    if (SUCCEEDED(SHCreateItemFromParsingName(initialDirectory.c_str(), 0, IID_IShellItem, reinterpret_cast<void**>(&shellItem))))
                        pfd->SetFolder(shellItem);
                    pfd->SetTitle(title.c_str());
                    if (SUCCEEDED(pfd->Show(GetActiveWindow()))) {
                        IShellItem *psi;
                        if (SUCCEEDED(pfd->GetResult(&psi))) {
                            LPWSTR lpwszName = NULL;
                            if(SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, (LPWSTR*)&lpwszName))) {
                                // Add directory path to the result
                                std::wstring wstrName(lpwszName);
                                ExtensionString pathName(wstrName);
                                ConvertToUnixPath(pathName);
                                selectedFiles->SetString(0, pathName);
                                ::CoTaskMemFree(lpwszName);
                            }
                            psi->Release();
                        }
                    }
                    if (shellItem != NULL)
                        shellItem->Release();
                }
                pfd->Release();
            }
        } else {
            // for XP, use the old-styled SHBrowseForFolder() implementation
            BROWSEINFO bi = {0};
            bi.hwndOwner = GetActiveWindow();
            bi.lpszTitle = title.c_str();
            bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
            bi.lpfn = SetInitialPathCallback;
            bi.lParam = (LPARAM)initialDirectory.c_str();

            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != 0) {
                if (SHGetPathFromIDList(pidl, szFile)) {
                    // Add directory path to the result
                    ExtensionString pathName(szFile);
                    ConvertToUnixPath(pathName);
                    selectedFiles->SetString(0, pathName);
                }
                IMalloc* pMalloc = NULL;
                SHGetMalloc(&pMalloc);
                if (pMalloc) {
                    pMalloc->Free(pidl);
                    pMalloc->Release();
                }
            }
        }
    } else {
        OPENFILENAME ofn;

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.hwndOwner = GetActiveWindow();
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_UNC_PATH;
        ofn.lpstrTitle = title.c_str();

        // TODO (issue #65) - Use passed in file types. Note, when fileTypesStr is null, all files should be shown
        /* findAndReplaceString( fileTypesStr, std::string(" "), std::string(";*."));
        LPCWSTR allFilesFilter = L"All Files\0*.*\0\0";*/

        ofn.lpstrFilter = L"All Files\0*.*\0Web Files\0*.js;*.css;*.htm;*.html\0\0";
           
        ofn.lpstrInitialDir = initialDirectory.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
        if (allowMultipleSelection)
            ofn.Flags |= OFN_ALLOWMULTISELECT;

        if (GetOpenFileName(&ofn)) {
            if (allowMultipleSelection) {
                // Multiple selection encodes the files differently

                // If multiple files are selected, the first null terminator
                // signals end of directory that the files are all in
                std::wstring dir(szFile);

                // Check for two null terminators, which signal that only one file
                // was selected
                if (szFile[dir.length() + 1] == '\0') {
                    ExtensionString filePath(dir);
                    ConvertToUnixPath(filePath);
                    selectedFiles->SetString(0, filePath);
                } else {
                    // Multiple files are selected
                    wchar_t fullPath[MAX_UNC_PATH];
                    for (int i = (dir.length() + 1), fileIndex = 0; ; fileIndex++) {
                        // Get the next file name
                        std::wstring file(&szFile[i]);

                        // Two adjacent null characters signal the end of the files
                        if (file.length() == 0)
                            break;

                        // The filename is relative to the directory that was specified as
                        // the first string
                        if (PathCombine(fullPath, dir.c_str(), file.c_str()) != NULL) {
                            ExtensionString filePath(fullPath);
                            ConvertToUnixPath(filePath);
                            selectedFiles->SetString(fileIndex, filePath);
                        }

                        // Go to the start of the next file name
                        i += file.length() + 1;
                    }
                }

            } else {
                // If multiple files are not allowed, add the single file
                std::wstring filePath(szFile);
                ConvertToUnixPath(filePath);
                selectedFiles->SetString(0, filePath);
            }
        }
    }

    return NO_ERROR;
}

int32 ShowSaveDialog(ExtensionString title,
                       ExtensionString initialDirectory,
                       ExtensionString proposedNewFilename,
                       ExtensionString& absoluteFilepath)
{
    ConvertToNativePath(initialDirectory);        // Windows common file dlgs require Windows-style paths

    // call Windows common file-saveas dialog to prompt for a filename in a writeable location
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.hwndOwner = GetActiveWindow();
    ofn.lStructSize = sizeof(ofn);
    wchar_t szFile[MAX_UNC_PATH];
    wcscpy(szFile, proposedNewFilename.c_str());
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_UNC_PATH;
    ofn.lpstrFilter = L"All Files\0*.*\0Web Files\0*.js;*.css;*.htm;*.html\0Text Files\0*.txt\0\0";
    ofn.lpstrInitialDir = initialDirectory.c_str();
    ofn.Flags = OFN_ENABLESIZING | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (GetSaveFileName(&ofn)) {
        // return the validated filename using Unix-style paths
        absoluteFilepath = ofn.lpstrFile;
        ConvertToUnixPath(absoluteFilepath);
    }

    return NO_ERROR;
}

int32 IsNetworkDrive(ExtensionString path, bool& isRemote)
{
    if (path.length() == 0) {
        return ERR_INVALID_PARAMS;
    }

    DWORD dwAttr;
    dwAttr = GetFileAttributes(path.c_str());
    if (INVALID_FILE_ATTRIBUTES == dwAttr)
        return ConvertWinErrorCode(GetLastError());

    ExtensionString drive = path.substr(0, path.find('/') + 1);
    isRemote = GetDriveType(drive.c_str()) == DRIVE_REMOTE;

    return NO_ERROR;
}

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents)
{
    if (path.length() && path[path.length() - 1] != '/')
        path += '/';

    path += '*';

    // Convert to native path to ensure that FindFirstFile and FindNextFile
    // function correctly for all paths including paths to a network drive.
    ConvertToNativePath(path);

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(path.c_str(), &ffd);

    std::vector<ExtensionString> resultFiles;
    std::vector<ExtensionString> resultDirs;

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Ignore '.' and '..' and system files
            if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L"..") ||
                (ffd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
                continue;

            // Collect file and directory names separately
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                resultDirs.push_back(ExtensionString(ffd.cFileName));
            } else {
                resultFiles.push_back(ExtensionString(ffd.cFileName));
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);

        FindClose(hFind);
    } 
    else {
        return ConvertWinErrorCode(GetLastError());
    }

    // On Windows, list directories first, then files
    size_t i, total = 0;
    for (i = 0; i < resultDirs.size(); i++)
        directoryContents->SetString(total++, resultDirs[i]);
    for (i = 0; i < resultFiles.size(); i++)
        directoryContents->SetString(total++, resultFiles[i]);

    return NO_ERROR;
}

int32 MakeDir(ExtensionString path, int32 mode)
{
    // TODO (issue #1759): honor mode
    ConvertToNativePath(path);
    int err = SHCreateDirectoryEx(NULL, path.c_str(), NULL);

    return ConvertWinErrorCode(err);
}

int32 Rename(ExtensionString oldName, ExtensionString newName)
{
    if (!MoveFile(oldName.c_str(), newName.c_str()))
        return ConvertWinErrorCode(GetLastError());

    return NO_ERROR;
}

// function prototype for GetFinalPathNameByHandleW(), which is unavailable on Windows XP and earlier
typedef DWORD (WINAPI *PFNGFPNBH)(
  _In_   HANDLE hFile,
  _Out_  LPTSTR lpszFilePath,
  _In_   DWORD cchFilePath,
  _In_   DWORD dwFlags
);

int32 GetFileInfo(ExtensionString filename, uint32& modtime, bool& isDir, double& size, ExtensionString& realPath)
{

    WIN32_FILE_ATTRIBUTE_DATA   fad;
    if (!GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &fad)) {
        return ConvertWinErrorCode(GetLastError());
    }

    DWORD dwAttr = fad.dwFileAttributes;
    isDir = ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);

    modtime = FiletimeToTime(fad.ftLastWriteTime);

    LARGE_INTEGER size_tmp;
    size_tmp.HighPart = fad.nFileSizeHigh;
    size_tmp.LowPart = fad.nFileSizeLow;
    size = size_tmp.QuadPart;

    realPath = L"";
    if (dwAttr & FILE_ATTRIBUTE_REPARSE_POINT) {
        // conditionally call GetFinalPathNameByHandleW() if it's available -- Windows Vista or later
        HMODULE hDLL = ::GetModuleHandle(TEXT("kernel32.dll"));
        PFNGFPNBH pfn = (hDLL != NULL) ? (PFNGFPNBH)::GetProcAddress(hDLL, "GetFinalPathNameByHandleW") : NULL;
        if (pfn != NULL) {
            HANDLE      hFile;

            hFile = ::CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                wchar_t pathBuffer[MAX_UNC_PATH + 1];
                DWORD nChars;

                nChars = (*pfn)(hFile, pathBuffer, MAX_UNC_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
                if (nChars && nChars <= MAX_UNC_PATH) {
                    // Path returned by GetFilePathNameByHandle starts with "\\?\". Remove from returned value.
                    realPath = &pathBuffer[4];  

                    // UNC paths start with UNC. Update here, if needed.
                    if (realPath.find(L"UNC") == 0) {
                        realPath = L"\\" + ExtensionString(&pathBuffer[7]);
                    }

                    ConvertToUnixPath(realPath);
                }
                ::CloseHandle(hFile);
            }

            // Note: all realPath errors are ignored. If the realPath can't be determined, it should not make the
            // stat fail.
        }
    }

    return NO_ERROR;
}

const int BOMLength = 3;

typedef enum CheckedState { CS_UNKNOWN, CS_NO, CS_YES };

typedef struct UTFValidationState {

    UTFValidationState () {
        data        = NULL;
        dataLen     = 0;
        utf1632     = CS_UNKNOWN;
        preserveBOM = true;
    }

    char*            data;
    DWORD            dataLen;
    CheckedState     utf1632;
    bool             preserveBOM;
} UTFValidationState;

bool hasBOM(UTFValidationState& validationState)
{
    return ((validationState.dataLen >= BOMLength) && (validationState.data[0] == (char)0xEF) && (validationState.data[1] == (char)0xBB) && (validationState.data[2] == (char)0xBF));
}

bool hasUTF16_32(UTFValidationState& validationState)
{
    if (validationState.utf1632 == CS_UNKNOWN) {
        int flags = IS_TEXT_UNICODE_UNICODE_MASK|IS_TEXT_UNICODE_REVERSE_MASK;

        // Check to see if buffer is UTF-16 or UTF-32 with or without a BOM
        BOOL test = IsTextUnicode(validationState.data, validationState.dataLen, &flags);

        validationState.utf1632 = (test ? CS_YES : CS_NO);
    }

    return (validationState.utf1632 == CS_YES);
}

void RemoveBOM(UTFValidationState& validationState)
{
    if (!validationState.preserveBOM) {
        validationState.dataLen -= BOMLength;
        CopyMemory (validationState.data, validationState.data+3, validationState.dataLen);
    }
}

bool GetBufferAsUTF8(UTFValidationState& validationState)
{
    if (validationState.dataLen == 0) {
        return true;
    }    
    
    // if we know it's UTF-16 or UTF-32 then bail
    if (hasUTF16_32(validationState)) {
        return false;
    }

    // See if we can convert the data to UNICODE from UTF-8
    //  if the data isn't UTF-8, this will fail and the result will be 0
    int outBuffSize = (validationState.dataLen + 1) * 2;
    wchar_t* outBuffer = new wchar_t[outBuffSize];
    int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, validationState.data, validationState.dataLen, outBuffer, outBuffSize);
    delete []outBuffer;

    if ((result > 0) && hasBOM(validationState)) {
        RemoveBOM(validationState);
    }

    return (result > 0);
}

bool IsUTFLeadByte(char data)
{
   return (((data & 0xF8) == 0xF0) || // 4 BYTE
           ((data & 0xF0) == 0xE0) || // 3 BYTE
           ((data & 0xE0) == 0xC0));  // 2 BYTE
}

// we can't validate something that's smaller than 12 bytes 
const int kMinValidationLength = 12;

bool quickTestBufferForUTF8(UTFValidationState& validationState)
{
    if (validationState.dataLen < kMinValidationLength) {
        // we really don't know so just assume it's valid
        return true;
    }

    // if we know it's UTF-16 or UTF-32 then bail
    if (hasUTF16_32(validationState)) {
        return false;
    }

    // If it has a UTF-8 BOM, then 
    //  assume it's UTF8
    if (hasBOM(validationState)) {
        return true;
    }

    // find the last lead byte and truncate 
    //  the buffer beforehand and check that to avoid 
    //  checking a malformed data stream
    for (int i = 1; i < 4; i++) {
        int index = (validationState.dataLen - i);

        if ((index > 0) && (IsUTFLeadByte(validationState.data[index]))){
            validationState.dataLen = index;
            break;
        }

    }

    // this will check to see if the we have valid
    //  UTF8 data in the sample data.  This should tell
    //  us if it's binary or not but doesn't necessarily
    //  tell us if the file is valid UTF8
    return (GetBufferAsUTF8(validationState));
}


int32 ReadFile(ExtensionString filename, ExtensionString encoding, std::string& contents)
{
    if (encoding != L"utf8")
        return ERR_UNSUPPORTED_ENCODING;

    DWORD dwAttr;
    dwAttr = GetFileAttributes(filename.c_str());
    if (INVALID_FILE_ATTRIBUTES == dwAttr)
        return ConvertWinErrorCode(GetLastError());

    if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
        return ERR_CANT_READ;

    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    int32 error = NO_ERROR;

    if (INVALID_HANDLE_VALUE == hFile)
        return ConvertWinErrorCode(GetLastError()); 

    char* buffer = NULL;
    DWORD dwFileSize = GetFileSize(hFile, NULL);

    if (dwFileSize == 0) {
        contents = "";
    } else {
        DWORD dwBytesRead;
        
        // first just read a few bytes of the file
        //  to check for a binary or text format 
        //  that we can't handle

        // we just want to read enough to satisfy the
        //  UTF-16 or UTF-32 test with or without a BOM
        // the UTF-8 test could result in a false-positive
        //  but we'll check again with all bits if we 
        //  think it's UTF-8 based on just a few characters
        
        // if we're going to read fewer bytes than our
        //  quick test then we skip the quick test and just
        //  do the full test below since it will be fewer reads

        // We need a buffer that can handle UTF16 or UTF32 with or without a BOM 
        //  but with enough that we can test for true UTF data to test against 
        //  without reading partial character streams roughly 1000 characters 
        //  at UTF32 should do it:
        // 1000 chars + 32-bit BOM (UTF-32) = 4004 bytes 
        // 1001 chars without BOM  (UTF-32) = 4004 bytes 
        // 2001 chars + 16 bit BOM (UTF-16) = 4004 bytes 
        // 2002 chars without BOM  (UTF-16) = 4004 bytes 
        const DWORD quickTestSize = 4004; 
        static char quickTestBuffer[quickTestSize+1];

        UTFValidationState validationState;

        validationState.data = quickTestBuffer;
        validationState.dataLen = quickTestSize;

        if (dwFileSize > quickTestSize) {
            ZeroMemory(quickTestBuffer, sizeof(quickTestBuffer));
            if (ReadFile(hFile, quickTestBuffer, quickTestSize, &dwBytesRead, NULL)) {
                if (!quickTestBufferForUTF8(validationState)) {
                    error = ERR_UNSUPPORTED_ENCODING;
                }
                else {
                    // reset the file pointer back to beginning
                    //  since we're going to re-read the file wi
                    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
                }
            }
        }

        if (error == NO_ERROR) {
            // either we did a quick test and we think it's UTF-8 or 
            //  the file is small enough that we didn't spend the time
            //  to do a quick test so alloc the memory to read the entire
            //  file into memory and test it again...
            buffer = (char*)malloc(dwFileSize);
            if (buffer) {

                validationState.data = buffer;
                validationState.dataLen = dwFileSize;
                validationState.preserveBOM = false;

                if (ReadFile(hFile, buffer, dwFileSize, &dwBytesRead, NULL)) {
                    if (!GetBufferAsUTF8(validationState)) {
                        error = ERR_UNSUPPORTED_ENCODING;
                    } else {
                        contents = std::string(buffer, validationState.dataLen);
                    }        
                } else {
                    error = ConvertWinErrorCode(GetLastError(), false);
                }
                free(buffer);

            }
            else { 
                error = ERR_UNKNOWN;
            }
        }
    }
    CloseHandle(hFile);
    return error; 
}

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding)
{
    if (encoding != L"utf8")
        return ERR_UNSUPPORTED_ENCODING;

    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE,
        FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD dwBytesWritten;
    int error = NO_ERROR;

    if (INVALID_HANDLE_VALUE == hFile)
        return ConvertWinErrorCode(GetLastError(), false); 

    // TODO (issue 67) -  Should write to temp file and handle encoding
    if (!WriteFile(hFile, contents.c_str(), contents.length(), &dwBytesWritten, NULL)) {
        error = ConvertWinErrorCode(GetLastError(), false);
    }

    CloseHandle(hFile);
    return error;
}

int32 SetPosixPermissions(ExtensionString filename, int32 mode)
{
    DWORD dwAttr = GetFileAttributes(filename.c_str());

    if (dwAttr == INVALID_FILE_ATTRIBUTES)
        return ERR_NOT_FOUND;

    if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
        return NO_ERROR;

    bool write = (mode & 0200) != 0; 
    bool read = (mode & 0400) != 0;
    int mask = (write ? _S_IWRITE : 0) | (read ? _S_IREAD : 0);

    if (_wchmod(filename.c_str(), mask) == -1) {
        return ConvertErrnoCode(errno); 
    }

    return NO_ERROR;
}

int32 ShellDeleteFileOrDirectory(ExtensionString filename, bool allowUndo) 
{
    DWORD dwAttr = GetFileAttributes(filename.c_str());

    if (dwAttr == INVALID_FILE_ATTRIBUTES) {
        return ERR_NOT_FOUND;
    }

    // Windows XP doesn't like Posix Paths
    //  It will work but the filenames in the recycle
    //  bin are hidden so convert to a native path.
    ConvertToNativePath(filename);

    // Windows XP doesn't like directory names
    // that end with a trailing slash so remove it
    RemoveTrailingSlash(filename);

    WCHAR filepath[MAX_UNC_PATH+1] = {0};
    wcscpy(filepath, filename.c_str());

    SHFILEOPSTRUCT operation = {0};

    operation.wFunc = FO_DELETE;
    operation.hwnd = GetActiveWindow();
    operation.pFrom = filepath;
    operation.fFlags =  FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;

    if (allowUndo) {
        operation.fFlags |= FOF_ALLOWUNDO;
    }

    // error codes from this function are pretty vague
    // http://msdn.microsoft.com/en-us/library/windows/desktop/bb762164(v=vs.85).aspx
    // So for now, just treat all errors as ERR_UNKNOWN
    if (SHFileOperation(&operation)) {
        return ERR_UNKNOWN; 
    }
    return NO_ERROR;
}

int32 DeleteFileOrDirectory(ExtensionString filename)
{
    return ShellDeleteFileOrDirectory(filename, false);
}

void MoveFileOrDirectoryToTrash(ExtensionString filename, CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    int32 error = ShellDeleteFileOrDirectory(filename, true);

    response->GetArgumentList()->SetInt(1, error);
    browser->SendProcessMessage(PID_RENDERER, response);
}

void OnBeforeShutdown()
{
    LiveBrowserMgrWin::Shutdown();
}

void CloseWindow(CefRefPtr<CefBrowser> browser)
{
    if (browser.get()) {
        HWND browserHwnd = browser->GetHost()->GetWindowHandle();
        SetProp(browserHwnd, CLOSING_PROP, (HANDLE)1);
        browser->GetHost()->CloseBrowser(true);

        ::PostMessage(browser->IsPopup() ? browserHwnd : GetParent(browserHwnd),
                      WM_CLOSE, 0, 0);
    }
}

void BringBrowserWindowToFront(CefRefPtr<CefBrowser> browser)
{
    if (browser.get()) {
        HWND hwnd = browser->GetHost()->GetWindowHandle();
        if (hwnd) 
            ::BringWindowToTop(hwnd);
    }
}

void ConvertToNativePath(ExtensionString& filename)
{
    // Convert '/' to '\'
    replace(filename.begin(), filename.end(), '/', '\\');
}

void ConvertToUnixPath(ExtensionString& filename)
{
    // Convert '\\' to '/'
    replace(filename.begin(), filename.end(), '\\', '/');
}

bool isSlash(ExtensionString::value_type wc) { return wc == '/' || wc == '\\'; }

void RemoveTrailingSlash(ExtensionString& filename)
{
    int last = filename.length() - 1;
    if ((last >= 0) && isSlash(filename.at(last))) {
        filename.erase(last);
    }
}

// Maps errors from errno.h to the brackets error codes
// found in brackets_extensions.js
int ConvertErrnoCode(int errorCode, bool isReading)
{
    switch (errorCode) {
    case NO_ERROR:
        return NO_ERROR;
    case EINVAL:
        return ERR_INVALID_PARAMS;
    case ENOENT:
        return ERR_NOT_FOUND;
    default:
        return ERR_UNKNOWN;
    }
}

// Maps errors from  WinError.h to the brackets error codes
// found in brackets_extensions.js
int ConvertWinErrorCode(int errorCode, bool isReading)
{
    switch (errorCode) {
    case NO_ERROR:
        return NO_ERROR;
    case ERROR_PATH_NOT_FOUND:
    case ERROR_FILE_NOT_FOUND:
        return ERR_NOT_FOUND;
    case ERROR_ACCESS_DENIED:
        return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
    case ERROR_WRITE_PROTECT:
        return ERR_CANT_WRITE;
    case ERROR_HANDLE_DISK_FULL:
        return ERR_OUT_OF_SPACE;
    case ERROR_ALREADY_EXISTS:
        return ERR_FILE_EXISTS;
    default:
        return ERR_UNKNOWN;
    }
}

/**
 * Convert a FILETIME (number of 100 nanosecond intervals since Jan 1 1601)
 * to time_t (number of seconds since Jan 1 1970)
 */
time_t FiletimeToTime(FILETIME const& ft) {
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;

    // Convert the FILETIME from 100 nanosecond intervals into seconds
    // and then subtract the number of seconds between 
    // Jan 1 1601 and Jan 1 1970

    const int64 NANOSECOND_INTERVALS = 10000000ULL;
    const int64 SECONDS_FROM_JAN_1_1601_TO_JAN_1_1970 = 11644473600ULL;

    return ull.QuadPart / NANOSECOND_INTERVALS - SECONDS_FROM_JAN_1_1601_TO_JAN_1_1970;
}

int32 ShowFolderInOSWindow(ExtensionString pathname) {
    ConvertToNativePath(pathname);
    
    DWORD dwAttr = GetFileAttributes(pathname.c_str());
    if (dwAttr == INVALID_FILE_ATTRIBUTES) {
        return ConvertWinErrorCode(GetLastError());
    }
    
    if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        // Folder: open it directly, with nothing selected inside
        ShellExecute(NULL, L"open", pathname.c_str(), NULL, NULL, SW_SHOWDEFAULT);
        
    } else {
        // File: open its containing folder with this file selected
        ITEMIDLIST *pidl = ILCreateFromPath(pathname.c_str());
        if (pidl) {
            SHOpenFolderAndSelectItems(pidl,0,0,0);
            ILFree(pidl);
        }
    }
    
    return NO_ERROR;
}

int32 CopyFile(ExtensionString src, ExtensionString dest)
{
    ConvertToNativePath(src);
    ConvertToNativePath(dest);

    if (!CopyFile(src.c_str(), dest.c_str(), FALSE)) {
        return ConvertWinErrorCode(GetLastError());
    }
    return NO_ERROR;
}

int32 GetPendingFilesToOpen(ExtensionString& files) {
    files = gFilesToOpen;
    ConvertToUnixPath(files);
    gFilesToOpen = L"";
    return NO_ERROR;
}

// Return index where menu or menu item should be placed.
// -1 indicates append. -2 indicates 'before' - WINAPI supports 
// placing a menu before an item without needing the position.

const int kAppend = -1;
const int kBefore = -2;

int32 GetMenuPosition(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId, ExtensionString& parentId, int& index)
{
    index = -1;
    parentId = ExtensionString();
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(commandId);

    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }

    HMENU parentMenu = (HMENU)NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    if (parentMenu == NULL) {
        parentMenu = GetMenu((HWND)getMenuParent(browser));
    } else {
        parentId = NativeMenuModel::getInstance(getMenuParent(browser)).getParentId(tag);
    }

    int len = GetMenuItemCount(parentMenu);
    for (int i = 0; i < len; i++) {
        MENUITEMINFO parentItemInfo;
        memset(&parentItemInfo, 0, sizeof(MENUITEMINFO));
        parentItemInfo.cbSize = sizeof(MENUITEMINFO);
        parentItemInfo.fMask = MIIM_ID;
        
        if (!GetMenuItemInfo(parentMenu, i, TRUE, &parentItemInfo)) {
            int err = GetLastError();
            return ConvertErrnoCode(err);
        }
        if (parentItemInfo.wID == (UINT)tag) {
            index = i;
            return NO_ERROR;
        }
    }

    return ERR_NOT_FOUND;
}

bool isSeparator(HMENU menu, int32 idx)
{
    MENUITEMINFO itemInfo = {0};
    itemInfo.cbSize = sizeof(MENUITEMINFO);
    itemInfo.fMask = MIIM_FTYPE;
    if (!GetMenuItemInfo(menu, idx, TRUE, &itemInfo)) {
        return FALSE;
    }
    return (itemInfo.fType & MFT_SEPARATOR) != 0;
}

HMENU getMenu(CefRefPtr<CefBrowser> browser, const ExtensionString& id)
{
    NativeMenuModel model = NativeMenuModel::getInstance(getMenuParent(browser));
    int32 tag = model.getTag(id);

    MENUITEMINFO parentItemInfo = {0};
    parentItemInfo.cbSize = sizeof(MENUITEMINFO);
    parentItemInfo.fMask = MIIM_SUBMENU;
    if (!GetMenuItemInfo((HMENU)model.getOsItem(tag), tag, FALSE, &parentItemInfo)) {
        return 0;
    }
    return parentItemInfo.hSubMenu;
}

int32 getNewMenuPosition(CefRefPtr<CefBrowser> browser, const ExtensionString& parentId, const ExtensionString& position, const ExtensionString& relativeId, int32& positionIdx)
{    
    int32 errCode = NO_ERROR;
    ExtensionString pos = position;
    ExtensionString relId = relativeId;
    NativeMenuModel model = NativeMenuModel::getInstance(getMenuParent(browser));

    if (pos.size() == 0)
    {
        positionIdx = kAppend;
    } else if (pos == L"first") {
        positionIdx = 0;
    } else if (pos == L"firstInSection" || pos == L"lastInSection") {
        int32 startTag = model.getTag(relId);
        HMENU startItem = (HMENU)model.getOsItem(startTag);
        ExtensionString startParentId = model.getParentId(startTag);
        HMENU parentMenu = (HMENU)model.getOsItem(model.getTag(startParentId));
        int32 idx;

        if (startParentId != parentId) {
            // Section is in a different menu
            positionIdx = -1;
            return ERR_NOT_FOUND;
        }

        parentMenu = getMenu(browser, startParentId);

        GetMenuPosition(browser, relId, startParentId, idx);

        if (pos == L"firstInSection") {
            // Move backwards until reaching the beginning of the menu or a separator
            while (idx >= 0) {
                if (isSeparator(parentMenu, idx)) {
                    break;
                }
                idx--;
            }
            if (idx < 0) {
                positionIdx = 0;
            } else {
                idx++;
                pos = L"before";
            }
        } else { // "lastInSection"
            int32 numItems = GetMenuItemCount(parentMenu);
            // Move forwards until reaching the end of the menu or a separator
            while (idx < numItems) {
                if (isSeparator(parentMenu, idx)) {
                    break;
                }
                idx++;
            }
            if (idx == numItems) {
                positionIdx = -1;
            } else {
                idx--;
                pos = L"after";
            }
        }

        if (pos == L"before" || pos == L"after") {
            MENUITEMINFO itemInfo = {0};
            itemInfo.cbSize = sizeof(MENUITEMINFO);
            itemInfo.fMask = MIIM_ID;
                
            if (!GetMenuItemInfo(parentMenu, idx, TRUE, &itemInfo)) {
                int err = GetLastError();
                return ConvertErrnoCode(err);
            }
            relId = model.getCommandId(itemInfo.wID);
        }
    } 
    
    if ((pos == L"before" || pos == L"after") && relId.size() > 0) {
        if (pos == L"before") {
            positionIdx = kBefore;
        } else {
            positionIdx = kAppend;
        }

        ExtensionString newParentId;
        errCode = GetMenuPosition(browser, relId, newParentId, positionIdx);

        if (parentId.size() > 0 && parentId != newParentId) {
            errCode = ERR_NOT_FOUND;
        }

        // If we don't find the relative ID, return an error and
        // set positionIdx to kAppend. The item will be appended and an
        // error will be shown in the console.
        if (errCode == ERR_NOT_FOUND) {
            positionIdx = kAppend;
        } else if (positionIdx >= 0 && pos == L"after") {
            positionIdx += 1;
        }
    }

    return errCode;
}

int32 AddMenu(CefRefPtr<CefBrowser> browser, ExtensionString itemTitle, ExtensionString command,
              ExtensionString position, ExtensionString relativeId)
{
    HWND mainWindow = (HWND)getMenuParent(browser);
    HMENU mainMenu = GetMenu(mainWindow);
    if (mainMenu == NULL) {
        mainMenu = CreateMenu();
        SetMenu(mainWindow, mainMenu);
    }

    int32 tag = NativeMenuModel::getInstance(mainWindow).getTag(command);
    if (tag == kTagNotFound) {
        tag = NativeMenuModel::getInstance(mainWindow).getOrCreateTag(command, ExtensionString());
        NativeMenuModel::getInstance(mainWindow).setOsItem(tag, (void*)mainMenu);
    } else {
        // menu is already there
        return NO_ERROR;
    }

    bool inserted = false;    
    int32 positionIdx;
    int32 errCode = getNewMenuPosition(browser, L"", position, relativeId, positionIdx);

    MENUITEMINFO menuInfo;
    memset(&menuInfo, 0, sizeof(MENUITEMINFO));
    HMENU newMenu = CreateMenu();
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.wID = (UINT)tag;
    menuInfo.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_FTYPE;    

#ifdef DARK_UI
    menuInfo.fType = MFT_OWNERDRAW;
#else
    menuInfo.fType = MFT_STRING;
#endif
    menuInfo.dwTypeData = (LPWSTR)itemTitle.c_str();
    menuInfo.cch = itemTitle.size();        
        
    if (positionIdx == kAppend) {
        if (!InsertMenuItem(mainMenu, -1, TRUE, &menuInfo)) {
            return ConvertErrnoCode(GetLastError());
        }
    }
    else if (positionIdx >= 0) {
        if (!InsertMenuItem(mainMenu, positionIdx, TRUE, &menuInfo)) {
            return ConvertErrnoCode(GetLastError());
        }
    }
    else
    {
        int32 relativeTag = NativeMenuModel::getInstance(mainWindow).getTag(relativeId);
        if (!InsertMenuItem(mainMenu, relativeTag, FALSE, &menuInfo)) {
            return ConvertErrnoCode(GetLastError());
        }
    }
    ::SendMessage(mainWindow, WM_USER+1004, 0, 0);
    return errCode;
}

// Return true if the unicode character is one of the symbols that can be used 
// directly as an accelerator key without converting it to its virtual key code.
// Currently, we have the unicode minus symbol and up/down, left/right arrow keys.
bool canBeUsedAsShortcutKey(int unicode)
{
    return (unicode == UNICODE_MINUS || (unicode >= UNICODE_LEFT_ARROW && unicode <= UNICODE_DOWN_ARROW));
}

bool UpdateAcceleratorTable(int32 tag, ExtensionString& keyStr)
{
    int keyStrLen = keyStr.length();
    if (keyStrLen) {
        LPACCEL lpaccelNew;             // pointer to new accelerator table
        HACCEL haccelOld;               // handle to old accelerator table
        int numAccelerators = 0;        // number of accelerators in table
        BYTE fAccelFlags = FVIRTKEY;    // fVirt flags for ACCEL structure 

        // Save the current accelerator table. 
        haccelOld = hAccelTable; 

        // Count the number of entries in the current 
        // table, allocate a buffer for the table, and 
        // then copy the table into the buffer.
        numAccelerators = CopyAcceleratorTable(haccelOld, NULL, 0); 
        numAccelerators++; // need room for one more
        lpaccelNew = (LPACCEL) LocalAlloc(LPTR, numAccelerators * sizeof(ACCEL)); 

        if (lpaccelNew != NULL) 
        {
            CopyAcceleratorTable(hAccelTable, lpaccelNew, numAccelerators); 
        }

        // look at the passed-in key, pull out modifiers, etc.
        if (keyStrLen > 1) {
            std::transform(keyStr.begin(), keyStr.end(), keyStr.begin(), ::toupper);
            if (keyStr.find(L"CMD") != ExtensionString::npos ||
                keyStr.find(L"CTRL") != ExtensionString::npos) {
                    fAccelFlags |= FCONTROL;
            }

            if (keyStr.find(L"ALT") != ExtensionString::npos) {
                fAccelFlags |= FALT;
            }

            if (keyStr.find(L"SHIFT") != ExtensionString::npos) {
                fAccelFlags |= FSHIFT;
            }
        }

        // Add the new accelerator to the end of the table
        UINT newItem = numAccelerators - 1;

        lpaccelNew[newItem].cmd = (WORD) tag;
        lpaccelNew[newItem].fVirt = fAccelFlags;
        if (keyStr.find(L"BACKSPACE")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_BACK;
        } else if (keyStr.find(L"DEL")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_DELETE;
        } else if (keyStr.find(L"SPACE")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_SPACE;
        } else if (keyStr.find(L"TAB")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_TAB;
        } else if (keyStr.find(L"ENTER")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_RETURN;
        }  else if (keyStr.find(L"PAGEUP")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_PRIOR;
        }  else if (keyStr.find(L"PAGEDOWN")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_NEXT;
        }  else if (keyStr.find(L"UP")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_UP;
        }  else if (keyStr.find(L"DOWN")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_DOWN;
        }  else if (keyStr.find(L"LEFT")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_LEFT;
        }  else if (keyStr.find(L"RIGHT")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_RIGHT;
        }  else if (keyStr.find(L"INSERT")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_INSERT;
        }  else if (keyStr.find(L"HOME")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_HOME;
        }  else if (keyStr.find(L"END")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_END;
        }  else if (keyStr.find(L"ESC")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_ESCAPE;
        } else {
            bool isFunctionKey = false;
            WCHAR fKey[4];

            // Check for F1 to F15 function keys. Note that we have to count down here because 
            // we want F12 and such to be found before F1 and so on.
            for (int i = VK_F15; i >= VK_F1; i--) {
                swprintf(fKey, sizeof(fKey), L"F%d", (i - VK_F1 + 1));
            
                if (keyStr.find(fKey) != ExtensionString::npos) {
                    lpaccelNew[newItem].key = i;
                    isFunctionKey = true;
                    break;
                }
            }

            if (!isFunctionKey) {
                int ascii = keyStr.at(keyStrLen-1);
 
                if  ( canBeUsedAsShortcutKey(ascii) || (ascii < 128 && isalnum(ascii)) ) {
                    lpaccelNew[newItem].key = ascii;
                } else {
                    // Get the virtual key code for non-alpha-numeric characters.
                    SHORT keyCode = ::VkKeyScan(ascii);
                    WORD vKey = (WORD)(keyCode & 0xFF);
                    bool isAltGr = ((keyCode & 0x600) == 0x600);

                    // Get unshifted key from keyCode so that we can determine whether the 
                    // key is a shifted one or not.
                    UINT unshiftedChar = ::MapVirtualKey(vKey, MAPVK_VK_TO_CHAR);    
                    bool isDeadKey = ((unshiftedChar & 0x80000000) == 0x80000000);
  
                    // If one of the following is found, then the shortcut is not available for the
                    // current keyboard layout. 
                    //
                    //     * keyCode is -1 -- meaning the key is not available on the current keyboard layout
                    //     * is a dead key -- a key used to attach a specific diacritic to a base letter. 
                    //     * is altGr character -- the character is only available when Ctrl and Alt keys are also pressed together.
                    //     * unshiftedChar is 0 -- meaning the key is not available on the current keyboard layout
                    //     * The key is a shifted character sharing with one of the number keys on the keyboard. An example 
                    //       of this is the '/' key on German keyboard layout. It is a shifted key on number key '7'.
                    // 
                    // So don't update the accelerator table. Just return false here so that the
                    // caller can remove the shortcut string from the menu title. 
                    if (keyCode == -1 || isDeadKey || isAltGr || unshiftedChar == 0 ||
                        (unshiftedChar >= '0' && unshiftedChar <= '9')) {
                        LocalFree(lpaccelNew);
                        return false;
                    }
                    
                    lpaccelNew[newItem].key = vKey;
                }
            }
        }

        bool existingOne = false;
        for (int i = 0; i < numAccelerators - 1; i++) {
            if (memcmp(&lpaccelNew[i], &lpaccelNew[newItem], sizeof(ACCEL)) == 0) {
                existingOne = true;
                break;
            }
        }

        if (!existingOne) {
            // Create the new accelerator table, and 
            // destroy the old one.
            DestroyAcceleratorTable(haccelOld); 
            hAccelTable = CreateAcceleratorTable(lpaccelNew, numAccelerators);
        }
        LocalFree(lpaccelNew);
    }

    return true;
}

int32 RemoveKeyFromAcceleratorTable(int32 tag)
{
    LPACCEL lpaccelNew;             // pointer to new accelerator table
    HACCEL haccelOld;               // handle to old accelerator table
    int numAccelerators = 0;        // number of accelerators in table

    // Save the current accelerator table. 
    haccelOld = hAccelTable; 

    // Count the number of entries in the current 
    // table, allocate a buffer for the table, and 
    // then copy the table into the buffer.
    numAccelerators = CopyAcceleratorTable(haccelOld, NULL, 0);
    lpaccelNew = (LPACCEL) LocalAlloc(LPTR, numAccelerators * sizeof(ACCEL)); 

    if (lpaccelNew != NULL) 
    {
        CopyAcceleratorTable(hAccelTable, lpaccelNew, numAccelerators); 
    }

    // Move accelerator to the end of the table
    UINT newItem = numAccelerators - 1;
    for (int i = 0; i < numAccelerators; i++) {
        if (lpaccelNew[i].cmd == (WORD) tag) {
            lpaccelNew[i] = lpaccelNew[newItem];
            DestroyAcceleratorTable(haccelOld); 
            hAccelTable = CreateAcceleratorTable(lpaccelNew, numAccelerators - 1);
            return NO_ERROR;
        }
    }
    LocalFree(lpaccelNew);
    return ERR_NOT_FOUND;
}

ExtensionString GetDisplayKeyString(const ExtensionString& keyStr) 
{
    ExtensionString result = keyStr;

    // We get a keyStr that looks like "Ctrl-O", which is the format we
    // need for the accelerator table. For displaying in the menu, though,
    // we have to change it to "Ctrl+O". Careful not to change the final
    // character, though, so "Ctrl--" ends up as "Ctrl+-".
    if (result.size() > 0) {
        replace(result.begin(), result.end() - 1, '-', '+');
    }
    return result;
}

int32 AddMenuItem(CefRefPtr<CefBrowser> browser, ExtensionString parentCommand, ExtensionString itemTitle,
                  ExtensionString command, ExtensionString key, ExtensionString displayStr,
                  ExtensionString position, ExtensionString relativeId)
{
    int32 parentTag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(parentCommand);
    if (parentTag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }

    HMENU menu = (HMENU) NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(parentTag);
    if (menu == NULL)
        return ERR_NOT_FOUND;

    bool isSeparator = (itemTitle == L"---");
    HMENU submenu = NULL;
    MENUITEMINFO parentItemInfo;

    memset(&parentItemInfo, 0, sizeof(MENUITEMINFO));
    parentItemInfo.cbSize = sizeof(MENUITEMINFO);
    parentItemInfo.fMask = MIIM_ID | MIIM_DATA | MIIM_SUBMENU | MIIM_STRING;
    //get the menuitem containing this one, see if it has a sub menu. If it doesn't, add one.
    if (!GetMenuItemInfo(menu, parentTag, FALSE, &parentItemInfo)) {
        return ConvertErrnoCode(GetLastError());
    }
    if (parentItemInfo.hSubMenu == NULL) {
        parentItemInfo.hSubMenu = CreateMenu();
        parentItemInfo.fMask = MIIM_SUBMENU;
        if (!SetMenuItemInfo(menu, parentTag, FALSE, &parentItemInfo)) {
            return ConvertErrnoCode(GetLastError());        
        }
    }
    submenu = parentItemInfo.hSubMenu;
    int32 tag = -1;
    ExtensionString title;
    ExtensionString keyStr;
    ExtensionString displayKeyStr;

    tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(command);
    if (tag == kTagNotFound) {
        tag = NativeMenuModel::getInstance(getMenuParent(browser)).getOrCreateTag(command, parentCommand);
    } else {
        return NO_ERROR;
    }

    title = itemTitle.c_str();
    keyStr = key.c_str();

    if (key.length() > 0 && displayStr.length() == 0) {
        displayKeyStr = GetDisplayKeyString(keyStr);
    } else {
        displayKeyStr = displayStr;
    }

    if (displayKeyStr.length() > 0) {
        title = title + L"\t" + displayKeyStr;
    }

    // Add shortcut to the accelerator table first. If the shortcut cannot 
    // be added, then don't show it in the menu title.
    if (!isSeparator && !UpdateAcceleratorTable(tag, keyStr)) {
        title = title.substr(0, title.find('\t'));
    }

    int32 positionIdx;
    int32 errCode = getNewMenuPosition(browser, parentCommand, position, relativeId, positionIdx);
    bool inserted = false;
    if (positionIdx >= 0 || positionIdx == kBefore) 
    {
        MENUITEMINFO menuInfo;
        memset(&menuInfo, 0, sizeof(MENUITEMINFO));        
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.wID = (UINT)tag;
        menuInfo.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_FTYPE;    
        menuInfo.fType = MFT_STRING;
        if (isSeparator) {
            menuInfo.fType = MFT_SEPARATOR;
        }
        menuInfo.dwTypeData = (LPWSTR)title.c_str();
        menuInfo.cch = itemTitle.size();        
        if (positionIdx >= 0) {
            InsertMenuItem(submenu, positionIdx, TRUE, &menuInfo);
            inserted = true;
        }
        else
        {
            int32 relativeTag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(relativeId);
            if (relativeTag >= 0 && positionIdx == kBefore) {
                InsertMenuItem(submenu, relativeTag, FALSE, &menuInfo);
                inserted = true;
            } else {
                // menu is already there
                return NO_ERROR;
            }
        }
    }
    if (!inserted)
    {
        BOOL res;

        if (isSeparator) 
        {
            res = AppendMenu(submenu, MF_SEPARATOR, NULL, NULL);
        } else {
            res = AppendMenu(submenu, MF_STRING, tag, title.c_str());
        }

        if (!res) {
            return ConvertErrnoCode(GetLastError());
        }
    }
    
    NativeMenuModel::getInstance(getMenuParent(browser)).setOsItem(tag, (void*)submenu);

    return errCode;
}

int32 GetMenuItemState(CefRefPtr<CefBrowser> browser, ExtensionString commandId, bool& enabled, bool& checked, int& index) {
    static WCHAR titleBuf[MAX_LOADSTRING];  
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(commandId);
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }
    HMENU menu = (HMENU) NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    if (menu == NULL) {
        return ERR_NOT_FOUND;
    }
    SendMessage((HWND)getMenuParent(browser), WM_INITMENUPOPUP, (WPARAM)menu, 0);
    MENUITEMINFO itemInfo;
    memset(&itemInfo, 0, sizeof(MENUITEMINFO));
    itemInfo.cbSize = sizeof(MENUITEMINFO);
    itemInfo.fMask = MIIM_STATE;
    if (!GetMenuItemInfo(menu, tag, FALSE, &itemInfo)) {
        return ConvertErrnoCode(GetLastError());
    }
    enabled = (itemInfo.fState & MFS_DISABLED) == 0;
    checked = (itemInfo.fState & MFS_CHECKED) != 0;

    // For finding the index, we'll just walk through the menu items
    // until we find this one. Not super efficient, but this is just
    // intended for testing, anyway.

    int len = GetMenuItemCount(menu);
    for (int i = 0; i < len; i++) {
        memset(&itemInfo, 0, sizeof(MENUITEMINFO));
        itemInfo.cbSize = sizeof(MENUITEMINFO);
        itemInfo.fMask = MIIM_ID;

        if (!GetMenuItemInfo(menu, i, TRUE, &itemInfo)) {
            int err = GetLastError();
            return ConvertErrnoCode(err);
        }
        if (itemInfo.wID == (UINT)tag) {
            index = i;
            break;
        }
        index = -1;
    }

    return NO_ERROR;
}

void CALLBACK MenuRedrawTimerHandler(HWND hWnd, UINT uMsg, UINT_PTR idEvt, DWORD dwTime) {
    DrawMenuBar((HWND)getMenuParent(redrawBrowser));
    KillTimer(NULL, redrawTimerId);
    redrawTimerId = NULL;
    redrawBrowser = NULL;
}

// The menu bar needs to be redrawn, but we don't want to redraw with *every* change
// since that causes flicker if we're changing a bunch of titles in a row (like at
// app startup).  Set a timer here to minimize the amount of drawing.
void ScheduleMenuRedraw(CefRefPtr<CefBrowser> browser) {
    if (!redrawTimerId) {
        redrawBrowser = browser;
        redrawTimerId = SetTimer(NULL, redrawTimerId, kMenuRedrawTimeout, MenuRedrawTimerHandler);
    }
}

int GetMenuItemPosition(HMENU hMenu, UINT commandID)
{
    int count = GetMenuItemCount(hMenu);
    for (int i = 0; i < count; i++) {
        MENUITEMINFO mii;
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID;
        if (GetMenuItemInfo(hMenu, i, TRUE, &mii) && mii.wID == commandID)
            return i;
    }
    return -1;
}

int32 SetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString command, ExtensionString itemTitle) {
    static WCHAR titleBuf[MAX_LOADSTRING];

    // find the item
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(command);
    if (tag == kTagNotFound)
        return ERR_NOT_FOUND;

    HMENU menu = (HMENU) NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    if (menu == NULL)
        return ERR_NOT_FOUND;

    // change the title
    MENUITEMINFO itemInfo;
    memset(&itemInfo, 0, sizeof(MENUITEMINFO));
    itemInfo.cbSize = sizeof(MENUITEMINFO);
    itemInfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA | MIIM_SUBMENU | MIIM_STRING;
    itemInfo.cch = MAX_LOADSTRING;
    itemInfo.dwTypeData = titleBuf;
    if (!GetMenuItemInfo(menu, tag, FALSE, &itemInfo)) {
        return ConvertErrnoCode(GetLastError());
    }

    std::wstring shortcut(titleBuf);
    size_t pos = shortcut.find(L"\t");
    if (pos != -1) {
        shortcut = shortcut.substr(pos);
    } else {
        shortcut = L"";
    }

    std::wstring newTitle = itemTitle;
    if (shortcut.length() > 0) {
        newTitle += shortcut;
    }

    HMENU mainMenu = GetMenu((HWND)getMenuParent(browser));
    if (menu == mainMenu) 
    {
        // If we're changing the titles of top-level menu items
        //  we must remove and reinsert the menu so that the item
        //  can be remeasured otherwise the window doesn't get a 
        //  WM_MEASUREITEM to relayout the menu bar
        if (wcscmp(newTitle.c_str(), titleBuf) != 0) {
            int position = GetMenuItemPosition(mainMenu, (UINT)tag);

            RemoveMenu(mainMenu, tag, MF_BYCOMMAND);

            MENUITEMINFO menuInfo;
            memset(&menuInfo, 0, sizeof(MENUITEMINFO));

            menuInfo.cbSize = sizeof(MENUITEMINFO);
            menuInfo.wID = (UINT)tag;
            menuInfo.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_FTYPE | MIIM_SUBMENU;
#ifdef DARK_UI
            menuInfo.fType = MFT_OWNERDRAW;
#else
            menuInfo.fType = MFT_STRING;
#endif
            menuInfo.dwTypeData = (LPWSTR)newTitle.c_str();
            menuInfo.hSubMenu = itemInfo.hSubMenu;
            menuInfo.cch = newTitle.size();        
        
            InsertMenuItem(mainMenu, position, TRUE, &menuInfo);
        }

    } else { 
        itemInfo.fType = MFT_STRING; // just to make sure
        itemInfo.dwTypeData = (LPWSTR)newTitle.c_str();
        itemInfo.cch = newTitle.size();
        if (!SetMenuItemInfo(menu, tag, FALSE, &itemInfo)) {
            return ConvertErrnoCode(GetLastError());        
        }
    }

    // The menu bar needs to be redrawn
    ScheduleMenuRedraw(browser);

    return NO_ERROR;
}

int32 GetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString& title) {
    static WCHAR titleBuf[MAX_LOADSTRING];  
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(commandId);
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }
    HMENU menu = (HMENU) NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    if (menu == NULL) {
        return ERR_NOT_FOUND;
    }

    MENUITEMINFO itemInfo;
    memset(&itemInfo, 0, sizeof(MENUITEMINFO));
    itemInfo.cbSize = sizeof(MENUITEMINFO);
    itemInfo.fMask = MIIM_DATA | MIIM_STRING;
    itemInfo.dwTypeData = NULL;
    if (!GetMenuItemInfo(menu, tag, FALSE, &itemInfo)) {
        return ConvertErrnoCode(GetLastError());
    }

    // To get the title, we have to call GetMenuItemInfo again
    // if we have room in titleBuf, now get the menu item title
    if (++itemInfo.cch < MAX_LOADSTRING) {
        itemInfo.dwTypeData = titleBuf;
        BOOL res = GetMenuItemInfo(menu, tag, FALSE, &itemInfo);
        if (res && itemInfo.dwTypeData) {
            std::wstring titleStr = itemInfo.dwTypeData;
            size_t pos = titleStr.find(L"\t");
            if (pos != -1) {
                titleStr = titleStr.substr(0, pos);
            }
            title = titleStr;
        }
    }
    return NO_ERROR;
}

int32 SetMenuItemShortcut(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString shortcut, ExtensionString displayStr)
{
    NativeMenuModel model = NativeMenuModel::getInstance(getMenuParent(browser));
    int32 tag = model.getTag(commandId);
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }
    HMENU menu = (HMENU)model.getOsItem(tag);
    if (!menu) {
        return ERR_NOT_FOUND;
    }

    // Display string
    ExtensionString displayKeyStr;
    if (shortcut.length() > 0 && displayStr.length() == 0) {
        displayKeyStr = GetDisplayKeyString(shortcut);
    } else {
        displayKeyStr = displayStr;
    }

    // Remove old accelerator
    RemoveKeyFromAcceleratorTable(tag);

    // Set new
    if (shortcut.length() > 0 && !UpdateAcceleratorTable(tag, shortcut)) {
        return ERR_UNKNOWN;
    }

    // Update menu title
    static WCHAR titleBuf[MAX_LOADSTRING];  
    MENUITEMINFO itemInfo = {0};
    itemInfo.cbSize = sizeof(MENUITEMINFO);
    itemInfo.fMask = MIIM_DATA | MIIM_STRING;
    itemInfo.cch = MAX_LOADSTRING;
    itemInfo.dwTypeData = titleBuf;
    if (!GetMenuItemInfo(menu, tag, FALSE, &itemInfo)) {
        return ConvertErrnoCode(GetLastError());
    }

    std::wstring titleStr(titleBuf);
    size_t pos = titleStr.find(L"\t");
    if (pos != std::string::npos) {
        titleStr = titleStr.substr(0, pos);
    }
    if (displayKeyStr.size() > 0) {
        titleStr += L"\t";
        titleStr += displayKeyStr;
    }

    itemInfo.fType = MFT_STRING; // just to make sure
    itemInfo.dwTypeData = (LPWSTR)titleStr.c_str();
    itemInfo.cch = titleStr.size();

    if (!SetMenuItemInfo(menu, tag, FALSE, &itemInfo)) {
        return ConvertErrnoCode(GetLastError());        
    }

    return NO_ERROR;
}

int32 RemoveMenu(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId)
{
    // The menu bar needs to be redrawn
    ScheduleMenuRedraw(browser);

    return RemoveMenuItem(browser, commandId);
}

//Remove menu or menu item associated with commandId
int32 RemoveMenuItem(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId)
{
    int tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(commandId);
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }
    HMENU mainMenu = (HMENU)NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    if (mainMenu == NULL) {
        return ERR_NOT_FOUND;
    }
    DeleteMenu(mainMenu, tag, MF_BYCOMMAND);
    NativeMenuModel::getInstance(getMenuParent(browser)).removeMenuItem(commandId);
    RemoveKeyFromAcceleratorTable(tag);

    // The menu bar needs to be redrawn
    ScheduleMenuRedraw(browser);

    return NO_ERROR;
}

void DragWindow(CefRefPtr<CefBrowser> browser) {
    ReleaseCapture();
    HWND browserHwnd = (HWND)getMenuParent(browser);
    SendMessage(browserHwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}
    


