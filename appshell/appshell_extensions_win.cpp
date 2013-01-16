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
#include <stdio.h>
#include <sys/stat.h>

#define CLOSING_PROP L"CLOSING"
#define UNICODE_MINUS 0x2212
#define UNICODE_LEFT_ARROW 0x2190
#define UNICODE_DOWN_ARROW 0x2193


// Forward declarations for functions at the bottom of this file
void ConvertToNativePath(ExtensionString& filename);
void ConvertToUnixPath(ExtensionString& filename);
int ConvertErrnoCode(int errorCode, bool isReading = true);
int ConvertWinErrorCode(int errorCode, bool isReading = true);
static std::wstring GetPathToLiveBrowser();
static bool ConvertToShortPathName(std::wstring & path);
time_t FiletimeToTime(FILETIME const& ft);

extern HINSTANCE hInst;
extern HACCEL hAccelTable;

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

    UINT							m_closeLiveBrowserHeartbeatTimerId;
    UINT							m_closeLiveBrowserTimeoutTimerId;
    CefRefPtr<CefProcessMessage>	m_closeLiveBrowserCallback;
    CefRefPtr<CefBrowser>			m_browser;

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

    DWORD modulePathBufSize = _MAX_PATH+1;
    WCHAR modulePathBuf[_MAX_PATH+1];
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
       wchar_t wpath[MAX_PATH] = {0};

        DWORD length = MAX_PATH;
        RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)wpath, &length);
        RegCloseKey(hKey);

        return std::wstring(wpath);
    }

    // We didn't get an "App Paths" entry. This could be because Chrome was only installed for
    // the current user, or because Chrome isn't installed at all.
    // Look for Chrome.exe at C:\Users\{USERNAME}\AppData\Local\Google\Chrome\Application\chrome.exe
    TCHAR localAppPath[MAX_PATH] = {0};
    SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, localAppPath);
    std::wstring appPath(localAppPath);
    appPath += L"\\Google\\Chrome\\Application\\chrome.exe";
        
    return appPath;
}
    
static bool ConvertToShortPathName(std::wstring & path)
{
    DWORD shortPathBufSize = _MAX_PATH+1;
    WCHAR shortPathBuf[_MAX_PATH+1];
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

    if (enableRemoteDebugging)
        args += L" --remote-debugging-port=9222 --allow-file-access-from-files ";
    else
        args += L" ";
    args += argURL;

    // Args must be mutable
    int argsBufSize = args.length() +1;
    std::auto_ptr<WCHAR> argsBuf( new WCHAR[argsBufSize]);
    wcscpy(argsBuf.get(), args.c_str());

    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    //Send the whole command in through the args param. Windows will parse the first token up to a space
    //as the processes and feed the rest in as the argument string. 
    if (!CreateProcess(NULL, argsBuf.get(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
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
        // set a timeout for up to 3 minutes to close the browser 
        liveBrowserMgr->SetCloseTimeoutTimerId( ::SetTimer(NULL, 0, 3 * 60 * 1000, LiveBrowserMgrWin::CloseLiveBrowserTimerCallback) );
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
    wchar_t szFile[MAX_PATH];
    szFile[0] = 0;

    // TODO (issue #64) - This method should be using IFileDialog instead of the
    /* outdated SHGetPathFromIDList and GetOpenFileName.
       
    Useful function to parse fileTypesStr:
    template<class T>
    int inline findAndReplaceString(T& source, const T& find, const T& replace)
    {
    int num=0;
    int fLen = find.size();
    int rLen = replace.size();
    for (int pos=0; (pos=source.find(find, pos))!=T::npos; pos+=rLen)
    {
    num++;
    source.replace(pos, fLen, replace);
    }
    return num;
    }
    */

    // SHBrowseForFolder can handle Windows path only, not Unix path.
    // ofn.lpstrInitialDir also needs Windows path on XP and not Unix path.
    ConvertToNativePath(initialDirectory);

    if (chooseDirectory) {
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
    } else {
        OPENFILENAME ofn;

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.hwndOwner = GetActiveWindow();
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;

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
                    wchar_t fullPath[MAX_PATH];
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
                selectedFiles->SetString(0, szFile);
            }
        }
    }

    return NO_ERROR;
}

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents)
{
    if (path.length() && path[path.length() - 1] != '/')
        path += '/';

    path += '*';

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

int32 GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir)
{
    DWORD dwAttr = GetFileAttributes(filename.c_str());

    if (dwAttr == INVALID_FILE_ATTRIBUTES) {
        return ConvertWinErrorCode(GetLastError()); 
    }

    isDir = ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);

    WIN32_FILE_ATTRIBUTE_DATA   fad;
    if (!GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &fad)) {
        return ConvertWinErrorCode(GetLastError());
    }

    modtime = FiletimeToTime(fad.ftLastWriteTime);

    return NO_ERROR;
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
        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    int32 error = NO_ERROR;

    if (INVALID_HANDLE_VALUE == hFile)
        return ConvertWinErrorCode(GetLastError()); 

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    DWORD dwBytesRead;
    char* buffer = (char*)malloc(dwFileSize);
    if (buffer && ReadFile(hFile, buffer, dwFileSize, &dwBytesRead, NULL)) {
        contents = std::string(buffer, dwFileSize);
    }
    else {
        if (!buffer)
            error = ERR_UNKNOWN;
        else
            error = ConvertWinErrorCode(GetLastError());
    }
    CloseHandle(hFile);
    if (buffer)
        free(buffer);

    return error; 
}

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding)
{
    if (encoding != L"utf8")
        return ERR_UNSUPPORTED_ENCODING;

    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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

int32 DeleteFileOrDirectory(ExtensionString filename)
{
    DWORD dwAttr = GetFileAttributes(filename.c_str());

    if (dwAttr == INVALID_FILE_ATTRIBUTES)
        return ERR_NOT_FOUND;

    if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
        return ERR_NOT_FILE;

    if (!DeleteFile(filename.c_str()))
        return ConvertWinErrorCode(GetLastError());

    return NO_ERROR;
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
        browser->GetHost()->CloseBrowser();
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
    ShellExecute(NULL, L"open", pathname.c_str(), NULL, NULL, SW_SHOWDEFAULT);
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
                
        BOOL res = GetMenuItemInfo(parentMenu, i, TRUE, &parentItemInfo);
        if (res == 0) {
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

int32 getNewMenuPosition(CefRefPtr<CefBrowser> browser, const ExtensionString& position, const ExtensionString& relativeId, int32& errCode)
{    
    errCode = NO_ERROR;
    if (position.size() == 0)
    {
        return kAppend;
    }
    if (position == L"first") {
        return 0;
    }
    if (position == L"before" && relativeId.size() > 0) {
        return kBefore;
    }
    int32 positionIdx = kAppend;
    if (position == L"after" && relativeId.size() > 0) {
        ExtensionString parentId;   // unused variable
        errCode = GetMenuPosition(browser, relativeId, parentId, positionIdx);
        if (positionIdx != -1) {
            positionIdx++;
        }
    }

    return positionIdx;
}

int32 AddMenu(CefRefPtr<CefBrowser> browser, ExtensionString itemTitle, ExtensionString command,
              ExtensionString position, ExtensionString relativeId)
{
    HMENU mainMenu = GetMenu((HWND)getMenuParent(browser));
    if (mainMenu == NULL) {
        mainMenu = CreateMenu();
        SetMenu((HWND)getMenuParent(browser), mainMenu);
    }

    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(command);
    if (tag == kTagNotFound) {
        tag = NativeMenuModel::getInstance(getMenuParent(browser)).getOrCreateTag(command, ExtensionString());
        NativeMenuModel::getInstance(getMenuParent(browser)).setOsItem(tag, (void*)mainMenu);
    } else {
        // menu is already there
        return NO_ERROR;
    }

    bool inserted = false;    
    int32 errCode = NO_ERROR;
    int32 positionIdx = getNewMenuPosition(browser, position, relativeId, errCode);
    if (errCode != NO_ERROR) {
        return errCode;
    }
    if (positionIdx >= 0 || positionIdx == kBefore) 
    {
        MENUITEMINFO menuInfo;
        memset(&menuInfo, 0, sizeof(MENUITEMINFO));
        HMENU newMenu = CreateMenu();
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.wID = (UINT)tag;
        menuInfo.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING;    
        menuInfo.fType = MFT_STRING;
        menuInfo.dwTypeData = (LPWSTR)itemTitle.c_str();
        menuInfo.cch = itemTitle.size();        
        if (positionIdx >= 0) {
            InsertMenuItem(mainMenu, positionIdx, TRUE, &menuInfo);
            inserted = true;
        }
        else
        {
            int32 relativeTag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(relativeId);
            if (relativeTag >= 0 && positionIdx == kBefore) {
                InsertMenuItem(mainMenu, relativeTag, FALSE, &menuInfo);
                inserted = true;
            } else {
                // menu is already there
                return NO_ERROR;
            }
        }
    }
   
   if (inserted == false)
   {
        BOOL res = AppendMenu(mainMenu,MF_STRING, tag, itemTitle.c_str());
        
        if (res == 0) {
            return ConvertErrnoCode(GetLastError());
        }
   }
    return NO_ERROR;
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
        } else if (keyStr.find(L"TAB")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_TAB;
        } else if (keyStr.find(L"ENTER")  != ExtensionString::npos) {
            lpaccelNew[newItem].key = VK_RETURN;
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
                    int keyCode = ::VkKeyScan(ascii);
                    WORD vKey = (short)(keyCode & 0x000000FF);

                    // Get unshifted key from keyCode so that we can determine whether the 
                    // key is a shifted one or not.
                    UINT unshiftedChar = ::MapVirtualKey(vKey, 2);	
                    bool isDeadKey = ((unshiftedChar & 0x80000000) == 0x80000000);
  
                    // If key code is -1 or unshiftedChar is 0 or the key is a shifted key sharing with
                    // one of the number keys on the keyboard, then we don't have a functionable shortcut. 
                    // So don't update the accelerator table. Just return false here so that the
                    // caller can remove the shortcut string from the menu title. An example of this 
                    // is the '/' key on German keyboard layout. It is a shifted key on number key '7'.
                    if (keyCode == -1 || isDeadKey || unshiftedChar == 0 ||
                        (unshiftedChar >= '0' && unshiftedChar <= '9')) {
                        LocalFree(lpaccelNew);
                        return false;
                    }
                    
                    lpaccelNew[newItem].key = vKey;
                }
            }
        }

        // Create the new accelerator table, and 
        // destroy the old one.
        DestroyAcceleratorTable(haccelOld); 
        hAccelTable = CreateAcceleratorTable(lpaccelNew, numAccelerators);
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
        if (lpaccelNew[i].cmd = (WORD) tag) {
            lpaccelNew[i] = lpaccelNew[newItem];
            DestroyAcceleratorTable(haccelOld); 
            hAccelTable = CreateAcceleratorTable(lpaccelNew, numAccelerators - 1);
            return NO_ERROR;
        }
    }
    LocalFree(lpaccelNew);
    return ERR_NOT_FOUND;
}

int32 AddMenuItem(CefRefPtr<CefBrowser> browser, ExtensionString parentCommand, ExtensionString itemTitle,
                  ExtensionString command, ExtensionString key,
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
    BOOL res = GetMenuItemInfo(menu, parentTag, FALSE, &parentItemInfo);
    if (res == 0) {
        return ConvertErrnoCode(GetLastError());
    }
    if (parentItemInfo.hSubMenu == NULL) {
        parentItemInfo.hSubMenu = CreateMenu();
        parentItemInfo.fMask = MIIM_SUBMENU;
        res = SetMenuItemInfo(menu, parentTag, FALSE, &parentItemInfo);
        if (res == 0) {
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

    if (key.length() > 0) {
        // We get a keyStr that looks like "Ctrl-O", which is the format we
        // need for the accelerator table. For displaying in the menu, though,
        // we have to change it to "Ctrl+O". Careful not to change the final
        // character, though, so "Ctrl--" ends up as "Ctrl+-".
        displayKeyStr = keyStr;
        replace(displayKeyStr.begin(), displayKeyStr.end() - 1, '-', '+');

        title = title + L"\t" + displayKeyStr;
    }
    int32 errCode = NO_ERROR;
    int32 positionIdx = getNewMenuPosition(browser, position, relativeId, errCode);
    if (errCode != NO_ERROR) {
        return errCode;
    }
    bool inserted = false;
    if (positionIdx >= 0 || positionIdx == kBefore) 
    {
        MENUITEMINFO menuInfo;
        memset(&menuInfo, 0, sizeof(MENUITEMINFO));        
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.wID = (UINT)tag;
        menuInfo.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING;    
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
        if (isSeparator) 
        {
            res = AppendMenu(submenu, MF_SEPARATOR, NULL, NULL);
        } else {
            res = AppendMenu(submenu, MF_STRING, tag, title.c_str());
        }
    }
    
    if (res == 0) {
        return ConvertErrnoCode(GetLastError());
    }

    NativeMenuModel::getInstance(getMenuParent(browser)).setOsItem(tag, (void*)submenu);
    DrawMenuBar((HWND)getMenuParent(browser));

    if (!isSeparator && !UpdateAcceleratorTable(tag, keyStr)) {
        title = title.substr(0, title.find('\t'));
        SetMenuTitle(browser, command, title);
    }

    return NO_ERROR;
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
    BOOL res = GetMenuItemInfo(menu, tag, FALSE, &itemInfo);
    if (res == 0) {
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

        BOOL res = GetMenuItemInfo(menu, i, TRUE, &itemInfo);
        if (res == 0) {
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

int32 SetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString command, ExtensionString itemTitle) {

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
    itemInfo.fMask = MIIM_ID | MIIM_DATA | MIIM_SUBMENU | MIIM_STRING;
    BOOL res = GetMenuItemInfo(menu, tag, FALSE, &itemInfo);
    if (res == 0) {
        return ConvertErrnoCode(GetLastError());
    }

    itemInfo.fType = MFT_STRING; // just to make sure
    itemInfo.dwTypeData = (LPWSTR)itemTitle.c_str();
    itemInfo.cch = itemTitle.size();

    res = SetMenuItemInfo(menu, tag, FALSE, &itemInfo);
    if (res == 0) {
        return ConvertErrnoCode(GetLastError());        
    }

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
    BOOL res = GetMenuItemInfo(menu, tag, FALSE, &itemInfo);
    if (res == 0) {
        return ConvertErrnoCode(GetLastError());
    }

    // To get the title, we have to call GetMenuItemInfo again
    // if we have room in titleBuf, now get the menu item title
    if (++itemInfo.cch < MAX_LOADSTRING) {
        itemInfo.dwTypeData = titleBuf;
        res = GetMenuItemInfo(menu, tag, FALSE, &itemInfo);
        if (res && itemInfo.dwTypeData)
            title = itemInfo.dwTypeData;
    }
    return NO_ERROR;
}

int32 RemoveMenu(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId)
{
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
    DrawMenuBar((HWND)getMenuParent(browser));
    return NO_ERROR;
}


