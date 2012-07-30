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
#include "client_handler.h"

#include <algorithm>
#include <CommDlg.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <stdio.h>
#include <sys/stat.h>

#define CLOSING_PROP L"CLOSING"

extern CefRefPtr<ClientHandler> g_handler;

// Forward declarations for functions at the bottom of this file
void ConvertToNativePath(ExtensionString& filename);
void ConvertToUnixPath(ExtensionString& filename);
int ConvertErrnoCode(int errorCode, bool isReading = true);
int ConvertWinErrorCode(int errorCode, bool isReading = true);

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
    // Chrome.exe is at C:\Users\{USERNAME}\AppData\Local\Google\Chrome\Application\chrome.exe
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

    //When launching the app, we need to be careful about spaces in the path. A safe way to do this
    //is to use the shortpath. It doesn't look as nice, but it always works and never has a space
    if( !ConvertToShortPathName(appPath) ) {
        //If the shortpath failed, we need to bail since we don't know what to call now
        return ConvertWinErrorCode(GetLastError());
    }


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

/*
static bool IsChromeWindow(HWND hwnd)
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

struct EnumChromeWindowsCallbackData {
    bool    closeWindow;
    int     numberOfFoundWindows;
};

static BOOL CALLBACK EnumChromeWindowsCallback(HWND hwnd, LPARAM userParam)
{
    if( !hwnd ) {
        return FALSE;
    }

    EnumChromeWindowsCallbackData* cbData = reinterpret_cast<EnumChromeWindowsCallbackData*>(userParam);
    if(!cbData) {
        return FALSE;
    }

    if (!IsChromeWindow(hwnd)) {
        return TRUE;
    }

    cbData->numberOfFoundWindows++;
    //This window belongs to the instance of the browser we're interested in, tell it to close
    if( cbData->closeWindow ) {
        ::SendMessageCallback(hwnd, WM_CLOSE, NULL, NULL, CloseLiveBrowserAsyncCallback, NULL);
    }

    return TRUE;
}

static bool IsAnyChromeWindowsRunning() {
    EnumChromeWindowsCallbackData cbData = {0};
    cbData.numberOfFoundWindows = 0;
    cbData.closeWindow = false;
    ::EnumWindows(EnumChromeWindowsCallback, (LPARAM)&cbData);
    return( cbData.numberOfFoundWindows != 0 );
}

void CloseLiveBrowserKillTimers()
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

void CloseLiveBrowserFireCallback(int valToSend) {
    if (!m_closeLiveBrowserCallback.get() || !g_handler.get()) {
        return;
    }

    //kill the timers
    CloseLiveBrowserKillTimers();

    CefRefPtr<CefV8Context> context = g_handler->GetBrowser()->GetMainFrame()->GetV8Context();
    CefRefPtr<CefV8Value> objectForThis = context->GetGlobal();
    CefV8ValueList args;
    args.push_back( CefV8Value::CreateInt( valToSend ) );
    CefRefPtr<CefV8Value> r;
    CefRefPtr<CefV8Exception> e;

    m_closeLiveBrowserCallback->ExecuteFunctionWithContext( context , objectForThis, args, r, e, false );

    m_closeLiveBrowserCallback = NULL;
}

static void CALLBACK CloseLiveBrowserTimerCallback( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{        
    if( !s_instance ) {
        ::KillTimer(NULL, idEvent);
        return;
    }

    int retVal =  NO_ERROR;
    if( IsAnyChromeWindowsRunning() )
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

static void CALLBACK CloseLiveBrowserAsyncCallback( HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult )
{
    if( !s_instance ) {
        return;
    }

    //If there are no more versions of chrome, then fire the callback
    if( !IsAnyChromeWindowsRunning() ) {
        s_instance->CloseLiveBrowserFireCallback(NO_ERROR);
    }
    else if(s_instance->m_closeLiveBrowserHeartbeatTimerId == 0){
        //start a heartbeat timer to see if it closes after the message returned
        s_instance->m_closeLiveBrowserHeartbeatTimerId = ::SetTimer(NULL, 0, 30, CloseLiveBrowserTimerCallback);
    }
}

int CloseLiveBrowser()
{
    //We can only handle a single async callback at a time. If there is already one that hasn't fired then
    //we kill it now and get ready for the next. 
    m_closeLiveBrowserCallback = NULL;

    //Currently, brackets is mainly designed around a single main browser instance. We only support calling
    //back this function in that context. When we add support for multiple browser instances this will need
    //to update to get the correct context and track it's lifespan accordingly.
    if(!g_handler.get()) {
        return ERR_UNKNOWN;
    }

    if( ! g_handler->GetBrowser()->GetMainFrame()->GetV8Context()->IsSame(CefV8Context::GetCurrentContext()) ) {
        ASSERT(FALSE); //Getting called from not the main browser window.
        return ERR_UNKNOWN;
    }

    m_closeLiveBrowserCallback = arguments[0];

    EnumChromeWindowsCallbackData cbData = {0};

    cbData.numberOfFoundWindows = 0;
    cbData.closeWindow = true;
    ::EnumWindows(EnumChromeWindowsCallback, (LPARAM)&cbData);

    //set a timeout for up to 3 minutes to close the browser 
    UINT timeoutInMS = (cbData.numberOfFoundWindows == 0 ? USER_TIMER_MINIMUM : 3 * 60 * 1000);

    if( m_closeLiveBrowserCallback ) {
        m_closeLiveBrowserTimeoutTimerId = ::SetTimer(NULL, 0, timeoutInMS, CloseLiveBrowserTimerCallback);
    }

    return NO_ERROR;
}
*/

int32 ShowOpenDialog(bool allowMulitpleSelection,
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

    if (chooseDirectory) {
        // SHBrowseForFolder can handle Windows path only, not Unix path.
        ConvertToNativePath(initialDirectory);

        BROWSEINFO bi = {0};
        bi.hwndOwner = GetActiveWindow();
        bi.lpszTitle = title.c_str();
        bi.ulFlags = BIF_NEWDIALOGSTYLE;
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
        if (allowMulitpleSelection)
            ofn.Flags |= OFN_ALLOWMULTISELECT;

        if (GetOpenFileName(&ofn)) {
            if (allowMulitpleSelection) {
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
            // Ignore '.' and '..'
            if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L".."))
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

int32 GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir)
{
    DWORD dwAttr = GetFileAttributes(filename.c_str());

    if (dwAttr == INVALID_FILE_ATTRIBUTES) {
        return ConvertWinErrorCode(GetLastError()); 
    }

    isDir = ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);

    // Remove trailing "/", if present. _wstat will fail with a "file not found"
    // error if a directory has a trailing '/' in the name.
    if (filename[filename.length() - 1] == '/')
        filename[filename.length() - 1] = 0;

    struct _stat buffer;
    if(_wstat(filename.c_str(), &buffer) == -1) {
        return ConvertErrnoCode(errno); 
    }

    modtime = buffer.st_mtime;

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

void CloseWindow(CefRefPtr<CefBrowser> browser)
{
    if (browser.get() && g_handler.get()) {
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
    default:
        return ERR_UNKNOWN;
    }
}

