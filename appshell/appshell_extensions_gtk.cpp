/*
 * Copyright (c) 2012 Chhatoi Pritam Baral <pritam@pritambaral.com>. All rights reserved.
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

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <X11/Xlib.h>

int ConvertLinuxErrorCode(int errorCode, bool isReading = true);

///////////////////////////////////////////////////////////////////////////////
// LiveBrowserMgrLin

class LiveBrowserMgrLin
{
public:
    static LiveBrowserMgrLin* GetInstance();
    static void Shutdown();

    bool IsChromeWindow(GtkWidget* hwnd);
    bool IsAnyChromeWindowsRunning();
    void CloseLiveBrowserKillTimers();
    void CloseLiveBrowserFireCallback(int valToSend);

    CefRefPtr<CefProcessMessage> GetCloseCallback() { return m_closeLiveBrowserCallback; }
    int GetCloseHeartbeatTimerId() { return m_closeLiveBrowserHeartbeatTimerId; }
    int GetCloseTimeoutTimerId() { return m_closeLiveBrowserTimeoutTimerId; }

    void SetCloseCallback(CefRefPtr<CefProcessMessage> closeLiveBrowserCallback)
        { m_closeLiveBrowserCallback = closeLiveBrowserCallback; }
    void SetBrowser(CefRefPtr<CefBrowser> browser)
        { m_browser = browser; }
    void SetCloseHeartbeatTimerId(int closeLiveBrowserHeartbeatTimerId)
        { m_closeLiveBrowserHeartbeatTimerId = closeLiveBrowserHeartbeatTimerId; }
    void SetCloseTimeoutTimerId(int closeLiveBrowserTimeoutTimerId)
        { m_closeLiveBrowserTimeoutTimerId = closeLiveBrowserTimeoutTimerId; }

private:
    // private so this class cannot be instantiated externally
    LiveBrowserMgrLin();
    virtual ~LiveBrowserMgrLin();

    int							m_closeLiveBrowserHeartbeatTimerId;
    int							m_closeLiveBrowserTimeoutTimerId;
    CefRefPtr<CefProcessMessage>	m_closeLiveBrowserCallback;
    CefRefPtr<CefBrowser>			m_browser;

    static LiveBrowserMgrLin* s_instance;
};

LiveBrowserMgrLin::LiveBrowserMgrLin()
    : m_closeLiveBrowserHeartbeatTimerId(0)
    , m_closeLiveBrowserTimeoutTimerId(0)
{
}

LiveBrowserMgrLin::~LiveBrowserMgrLin()
{
}

LiveBrowserMgrLin* LiveBrowserMgrLin::GetInstance()
{
    if (!s_instance)
        s_instance = new LiveBrowserMgrLin();
    return s_instance;
}

void LiveBrowserMgrLin::Shutdown()
{
    delete s_instance;
    s_instance = NULL;
}

bool LiveBrowserMgrLin::IsChromeWindow(GtkWidget* hwnd)
{
    if( !hwnd ) {
        return false;
    }

    //TODO be!!
    return true;
}

struct EnumChromeWindowsCallbackData
{
    bool    closeWindow;
    int     numberOfFoundWindows;
};

// bool CALLBACK LiveBrowserMgrLin::EnumChromeWindowsCallback(Window hwnd, long userParam)
// {
//     if( !hwnd || !s_instance) {
//         return FALSE;
//     }

//     EnumChromeWindowsCallbackData* cbData = reinterpret_cast<EnumChromeWindowsCallbackData*>(userParam);
//     if(!cbData) {
//         return FALSE;
//     }

//     if (!s_instance->IsChromeWindow(hwnd)) {
//         return TRUE;
//     }

//     cbData->numberOfFoundWindows++;
//     //This window belongs to the instance of the browser we're interested in, tell it to close
//     if( cbData->closeWindow ) {
//         ::SendMessageCallback(hwnd, WM_CLOSE, NULL, NULL, CloseLiveBrowserAsyncCallback, NULL);
//     }

//     return TRUE;
// }

bool LiveBrowserMgrLin::IsAnyChromeWindowsRunning()
{
    return false;
}

void LiveBrowserMgrLin::CloseLiveBrowserKillTimers()
{
    if (m_closeLiveBrowserHeartbeatTimerId) {
        m_closeLiveBrowserHeartbeatTimerId = 0;
    }

    if (m_closeLiveBrowserTimeoutTimerId) {
        m_closeLiveBrowserTimeoutTimerId = 0;
    }
}

void LiveBrowserMgrLin::CloseLiveBrowserFireCallback(int valToSend)
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

// void CALLBACK LiveBrowserMgrLin::CloseLiveBrowserTimerCallback( Window hwnd, int uMsg, int idEvent, int dwTime)
// {
//     if( !s_instance ) {
//         ::KillTimer(NULL, idEvent);
//         return;
//     }

//     int retVal =  NO_ERROR;
//     if( s_instance->IsAnyChromeWindowsRunning() )
//     {
//         retVal = ERR_UNKNOWN;
//         //if this is the heartbeat timer, wait for another beat
//         if (idEvent == s_instance->m_closeLiveBrowserHeartbeatTimerId) {
//             return;
//         }
//     }

//     //notify back to the app
//     s_instance->CloseLiveBrowserFireCallback(retVal);
// }

// void CALLBACK LiveBrowserMgrLin::CloseLiveBrowserAsyncCallback( Window hwnd, int uMsg, ULONG_PTR dwData, LRESULT lResult )
// {
//     if( !s_instance ) {
//         return;
//     }

//     //If there are no more versions of chrome, then fire the callback
//     if( !s_instance->IsAnyChromeWindowsRunning() ) {
//         s_instance->CloseLiveBrowserFireCallback(NO_ERROR);
//     }
//     else if(s_instance->m_closeLiveBrowserHeartbeatTimerId == 0){
//         //start a heartbeat timer to see if it closes after the message returned
//         s_instance->m_closeLiveBrowserHeartbeatTimerId = ::SetTimer(NULL, 0, 30, CloseLiveBrowserTimerCallback);
//     }
// }

LiveBrowserMgrLin* LiveBrowserMgrLin::s_instance = NULL;


// static int CALLBACK SetInitialPathCallback(Window hWnd, int uMsg, long lParam, long lpData)
// {
//     if (BFFM_INITIALIZED == uMsg && NULL != lpData)
//     {
//         SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
//     }

//     return 0;
// }

static const char* GetPathToLiveBrowser() 
{
    //#TODO Use execlp and be done with it! No need to reinvent the wheel; so badly that too!
    char *envPath = getenv( "PATH" ), *path, *dir, *currentPath;

    //# copy PATH and not modify the original
    path=(char *)malloc(strlen(envPath)+1);
    strcpy(path, envPath);

    // Prepend a forward-slash. For convenience
    const char* executable="/google-chrome";
    struct stat buf;
    int len;
 
    for ( dir = strtok( path, ":" ); dir; dir = strtok( NULL, ":" ) )
    {
        len=strlen(dir)+strlen(executable);
        // if((strrchr(dir,'/')-dir)==strlen(dir))
        // {
        //     currentPath = (char*)malloc(len);
        //     strcpy(currentPath,dir);
        // } else
        // {
        // stat handles consecutive forward slashes automatically. No need for above
            currentPath = (char *)malloc(len+1);
            strncpy(currentPath,dir,len);
        //}
        strcat(currentPath,executable);
    
        if(stat(currentPath,&buf)==0 && S_ISREG(buf.st_mode))
            return currentPath;
    }

    return "";
}

// static std::wstring GetPathToLiveBrowser() 
// {
//     HKEY hKey;

//     // First, look at the "App Paths" registry key for a "chrome.exe" entry. This only
//     // checks for installs for all users. If Chrome is only installed for the current user,
//     // we fall back to the code below.
//     if (ERROR_SUCCESS == RegOpenKeyEx(
//             HKEY_LOCAL_MACHINE, 
//             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe",
//             0, KEY_READ, &hKey)) {
//        wchar_t wpath[MAX_PATH] = {0};

//         int length = MAX_PATH;
//         RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)wpath, &length);
//         RegCloseKey(hKey);

//         return std::wstring(wpath);
//     }

//     // We didn't get an "App Paths" entry. This could be because Chrome was only installed for
//     // the current user, or because Chrome isn't installed at all.
//     // Look for Chrome.exe at C:\Users\{USERNAME}\AppData\Local\Google\Chrome\Application\chrome.exe
//     TCHAR localAppPath[MAX_PATH] = {0};
//     SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, localAppPath);
//     std::wstring appPath(localAppPath);
//     appPath += L"\\Google\\Chrome\\Application\\chrome.exe";
        
//     return appPath;
// }
    
// static bool ConvertToShortPathName(std::wstring & path)
// {
//     int shortPathBufSize = _MAX_PATH+1;
//     WCHAR shortPathBuf[_MAX_PATH+1];
//     int finalShortPathSize = ::GetShortPathName(path.c_str(), shortPathBuf, shortPathBufSize);
//     if( finalShortPathSize == 0 ) {
//         return false;
//     }
        
//     path.assign(shortPathBuf, finalShortPathSize);
//     return true;
// }

int32 OpenLiveBrowser(ExtensionString argURL, bool enableRemoteDebugging)
{    
    //# COnsider using execlp and avoid all this path mess!
    const char  *appPath = GetPathToLiveBrowser(),
                *arg1 = "--allow-file-access-from-files";
    std::string arg2(" ");

    if(enableRemoteDebugging)
        arg2.assign("--remote-debugging-port=9222");

    short int error=0;
    /* Using vfork() 'coz I need a shared variable for the error passing.
     * Do not replace with fork() unless you have a better way. */
    pid_t pid = vfork();
    switch(pid)
    {
        case -1:    //# Something went wrong
                return ConvertLinuxErrorCode(errno);
        case 0:     //# I'm the child. When I successfully exec, parent is resumed. Or when I _exec()
                execl(appPath, arg1, argURL.c_str(), arg2.c_str(),(char *)0);

                error=errno;
                _exit(0);
        default:
                if(error!=0)
                {
                    printf("Error!! %s\n", strerror(error));
                    return ConvertLinuxErrorCode(error);
                }
    }

    return NO_ERROR;
}

// int32 OpenLiveBrowser(ExtensionString argURL, bool enableRemoteDebugging)
// {
//     std::wstring appPath = GetPathToLiveBrowser();
//     std::wstring args = appPath;

//     if (enableRemoteDebugging)
//         args += L" --remote-debugging-port=9222 --allow-file-access-from-files ";
//     else
//         args += L" ";
//     args += argURL;

//     // Args must be mutable
//     int argsBufSize = args.length() +1;
//     std::auto_ptr<WCHAR> argsBuf( new WCHAR[argsBufSize]);
//     wcscpy(argsBuf.get(), args.c_str());

//     STARTUPINFO si = {0};
//     si.cb = sizeof(si);
//     PROCESS_INFORMATION pi = {0};

//     //Send the whole command in through the args param. Windows will parse the first token up to a space
//     //as the processes and feed the rest in as the argument string. 
//     if (!CreateProcess(NULL, argsBuf.get(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
//         return ConvertWinErrorCode(GetLastError());
//     }
        
//     CloseHandle(pi.hProcess);
//     CloseHandle(pi.hThread);

//     return NO_ERROR;
// }

void CloseLiveBrowser(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    //#TODO Duh,
}

// void CloseLiveBrowser(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
// {
//     LiveBrowserMgrLin* liveBrowserMgr = LiveBrowserMgrLin::GetInstance();
    
//     if (liveBrowserMgr->GetCloseCallback() != NULL) {
//         // We can only handle a single async callback at a time. If there is already one that hasn't fired then
//         // we kill it now and get ready for the next.
//         liveBrowserMgr->CloseLiveBrowserFireCallback(ERR_UNKNOWN);
//     }

//     liveBrowserMgr->SetCloseCallback(response);
//     liveBrowserMgr->SetBrowser(browser);

//     EnumChromeWindowsCallbackData cbData = {0};

//     cbData.numberOfFoundWindows = 0;
//     cbData.closeWindow = true;
//     ::EnumWindows(LiveBrowserMgrLin::EnumChromeWindowsCallback, (long)&cbData);

//     if (cbData.numberOfFoundWindows == 0) {
//         liveBrowserMgr->CloseLiveBrowserFireCallback(NO_ERROR);
//     } else if (liveBrowserMgr->GetCloseCallback()) {
//         // set a timeout for up to 3 minutes to close the browser 
//         liveBrowserMgr->SetCloseTimeoutTimerId( ::SetTimer(NULL, 0, 3 * 60 * 1000, LiveBrowserMgrLin::CloseLiveBrowserTimerCallback) );
//     }
// }

int32 OpenURLInDefaultBrowser(ExtensionString url)
{
    GError* error;
    gtk_show_uri(NULL, url.c_str(), GDK_CURRENT_TIME, &error);
    g_error_free(error);
    return NO_ERROR;
}

// int32 OpenURLInDefaultBrowser(ExtensionString url)
// {
//     int result = (int)ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

//     // If the result > 32, the function suceeded. If the result is <= 32, it is an
//     // error code.
//     if (result <= 32)
//         return ConvertWinErrorCode(result);

//     return NO_ERROR;
// }

int32 ShowOpenDialog(bool allowMultipleSelection,
                     bool chooseDirectory,
                     ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString fileTypes,
                     CefRefPtr<CefListValue>& selectedFiles)
{
    GtkWidget *dialog;
     const char* dialog_title = chooseDirectory ? "Open Directory" : "Open File";
     GtkFileChooserAction file_or_directory = chooseDirectory ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN ;
     dialog = gtk_file_chooser_dialog_new (dialog_title,
                          NULL,
                          file_or_directory,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                          NULL);
     
     if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
       {
         char *filename;
     
         filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
         printf("%s\n",filename);
         selectedFiles->SetString(0,filename);
         g_free (filename);
       }
     
     gtk_widget_destroy (dialog);
    return NO_ERROR;
}

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents)
{
    //# Add trailing /slash if neccessary
    if (path.length() && path[path.length() - 1] != '/')
        path += '/';

    DIR *dp;
    struct dirent *files;
    
    /*struct dirent
    {
        unsigned char  d_type; #not supported on all systems, faster than stat
        char d_name[]
    }*/
    

    if((dp=opendir(path.c_str()))==NULL)
        return ConvertLinuxErrorCode(errno,true);

    std::vector<ExtensionString> resultFiles;
    std::vector<ExtensionString> resultDirs;

    while((files=readdir(dp))!=NULL)
    {
        if(!strcmp(files->d_name,".") || !strcmp(files->d_name,".."))
            continue;
        if(files->d_type==DT_DIR)
            resultDirs.push_back(ExtensionString(files->d_name));
        else if(files->d_type==DT_REG)
            resultFiles.push_back(ExtensionString(files->d_name));
    }

    closedir(dp);

    //# List dirs first, files next
    size_t i, total = 0;
    for (i = 0; i < resultDirs.size(); i++)
        directoryContents->SetString(total++, resultDirs[i]);
    for (i = 0; i < resultFiles.size(); i++)
        directoryContents->SetString(total++, resultFiles[i]);

    return NO_ERROR;
}

int32 MakeDir(ExtensionString path, int mode)
{
    printf("in MakeDir trying to make %s\n", path.c_str());
    if(mkdir(path.c_str(),mode)==-1)
        return ConvertLinuxErrorCode(errno);

    return NO_ERROR;
}

// int32 MakeDir(ExtensionString path, int32 mode)
// {
//     // TODO (issue #1759): honor mode
//     ConvertToNativePath(path);
//     int err = SHCreateDirectoryEx(NULL, path.c_str(), NULL);

//     return ConvertWinErrorCode(err);
// }

int Rename(ExtensionString oldName, ExtensionString newName)
{
    if (rename(oldName.c_str(), newName.c_str())==-1)
        return ConvertLinuxErrorCode(errno);
}

// int32 Rename(ExtensionString oldName, ExtensionString newName)
// {
//     if (!MoveFile(oldName.c_str(), newName.c_str()))
//         return ConvertWinErrorCode(GetLastError());

//     return NO_ERROR;
// }

int GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir)
{
    struct stat buf;
    if(stat(filename.c_str(),&buf)==-1)
        return ConvertLinuxErrorCode(errno);

    modtime = buf.st_mtime;
    isDir = S_ISDIR(buf.st_mode);

    return NO_ERROR;
}

// int32 GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir)
// {
//     int dwAttr = GetFileAttributes(filename.c_str());

//     if (dwAttr == INVALID_FILE_ATTRIBUTES) {
//         return ConvertWinErrorCode(GetLastError()); 
//     }

//     isDir = ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);

//     WIN32_FILE_ATTRIBUTE_DATA   fad;
//     if (!GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &fad)) {
//         return ConvertWinErrorCode(GetLastError());
//     }

//     modtime = FiletimeToTime(fad.ftLastWriteTime);

//     return NO_ERROR;
// }

int ReadFile(ExtensionString filename, ExtensionString encoding, std::string& contents)
{

    if(encoding != "utf8")
        return NO_ERROR;    //#TODO ERR_UNSUPPORTED_ENCODING

    struct stat buf;
    if(stat(filename.c_str(),&buf)==-1)
        return ConvertLinuxErrorCode(errno);

    if(!S_ISREG(buf.st_mode))
        return NO_ERROR;    //# TODO ERR_CANT_READ when cleaninp up errors

    FILE* file = fopen(filename.c_str(),"r");
    if(file == NULL)
    {
        return ConvertLinuxErrorCode(errno);
    }

    fseek(file, 0, SEEK_END);
    long int size = ftell(file);
    rewind(file);

    char* content = (char*)calloc(size + 1, 1);

    fread(content,1,size,file);
    if(fclose(file)==EOF)
        return ConvertLinuxErrorCode(errno);

    contents = content;

    return NO_ERROR;
}

// int32 ReadFile(ExtensionString filename, ExtensionString encoding, std::string& contents)
// {
//     if (encoding != L"utf8")
//         return ERR_UNSUPPORTED_ENCODING;

//     int dwAttr;
//     dwAttr = GetFileAttributes(filename.c_str());
//     if (INVALID_FILE_ATTRIBUTES == dwAttr)
//         return ConvertWinErrorCode(GetLastError());

//     if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
//         return ERR_CANT_READ;

//     HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ,
//         0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//     int32 error = NO_ERROR;

//     if (INVALID_HANDLE_VALUE == hFile)
//         return ConvertWinErrorCode(GetLastError()); 

//     int dwFileSize = GetFileSize(hFile, NULL);
//     int dwBytesRead;
//     char* buffer = (char*)malloc(dwFileSize);
//     if (buffer && ReadFile(hFile, buffer, dwFileSize, &dwBytesRead, NULL)) {
//         contents = std::string(buffer, dwFileSize);
//     }
//     else {
//         if (!buffer)
//             error = ERR_UNKNOWN;
//         else
//             error = ConvertWinErrorCode(GetLastError());
//     }
//     CloseHandle(hFile);
//     if (buffer)
//         free(buffer);

//     return error; 
// }

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding)
{
    if(encoding != "utf8")
        return NO_ERROR;    //# TODO ERR_UNSUPPORTED_ENCODING;

    const char* content = contents.c_str();

    FILE* file = fopen(filename.c_str(),"w");
    if(file == NULL)
        return ConvertLinuxErrorCode(errno);

    long int size = strlen(content);

    fwrite(content,1,size,file);

    if(fclose(file)==EOF)
        return ConvertLinuxErrorCode(errno);

    return NO_ERROR;
}

int SetPosixPermissions(ExtensionString filename, int32 mode)
{
    if(chmod(filename.c_str(),mode)==-1)
        return ConvertLinuxErrorCode(errno);

    return NO_ERROR;
}

int DeleteFileOrDirectory(ExtensionString filename)
{
    if(unlink(filename.c_str())==-1)
        return ConvertLinuxErrorCode(errno);
    return NO_ERROR;
}

void CloseWindow(CefRefPtr<CefBrowser> browser)
{
    if (browser.get()) {
        browser->GetHost()->CloseBrowser();
    }
}

void BringBrowserWindowToFront(CefRefPtr<CefBrowser> browser)
{
    if (browser.get()) {
        GtkWindow* hwnd = GTK_WINDOW(browser->GetHost()->GetWindowHandle());
        if (hwnd) 
            gtk_window_present(hwnd);
    }
}

// void ConvertToNativePath(ExtensionString& filename)
// {
//     // Convert '/' to '\'
//     replace(filename.begin(), filename.end(), '/', '\\');
// }

// void ConvertToUnixPath(ExtensionString& filename)
// {
//     // Convert '\\' to '/'
//     replace(filename.begin(), filename.end(), '\\', '/');
// }

// // Maps errors from errno.h to the brackets error codes
// // found in brackets_extensions.js
// int ConvertErrnoCode(int errorCode, bool isReading)
// {
//     switch (errorCode) {
//     case NO_ERROR:
//         return NO_ERROR;
//     case EINVAL:
//         return ERR_INVALID_PARAMS;
//     case ENOENT:
//         return ERR_NOT_FOUND;
//     default:
//         return ERR_UNKNOWN;
//     }
// }

// // Maps errors from  WinError.h to the brackets error codes
// // found in brackets_extensions.js
// int ConvertWinErrorCode(int errorCode, bool isReading)
// {
//     switch (errorCode) {
//     case NO_ERROR:
//         return NO_ERROR;
//     case ERROR_PATH_NOT_FOUND:
//     case ERROR_FILE_NOT_FOUND:
//         return ERR_NOT_FOUND;
//     case ERROR_ACCESS_DENIED:
//         return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
//     case ERROR_WRITE_PROTECT:
//         return ERR_CANT_WRITE;
//     case ERROR_HANDLE_DISK_FULL:
//         return ERR_OUT_OF_SPACE;
//     case ERROR_ALREADY_EXISTS:
//         return ERR_FILE_EXISTS;
//     default:
//         return ERR_UNKNOWN;
//     }
// }

// /**
//  * Convert a FILETIME (number of 100 nanosecond intervals since Jan 1 1601)
//  * to time_t (number of seconds since Jan 1 1970)
//  */
// time_t FiletimeToTime(FILETIME const& ft) {
//     ULARGE_INTEGER ull;
//     ull.LowPart = ft.dwLowDateTime;
//     ull.HighPart = ft.dwHighDateTime;

//     // Convert the FILETIME from 100 nanosecond intervals into seconds
//     // and then subtract the number of seconds between 
//     // Jan 1 1601 and Jan 1 1970

//     const int64 NANOSECOND_INTERVALS = 10000000ULL;
//     const int64 SECONDS_FROM_JAN_1_1601_TO_JAN_1_1970 = 11644473600ULL;

//     return ull.QuadPart / NANOSECOND_INTERVALS - SECONDS_FROM_JAN_1_1601_TO_JAN_1_1970;
// }

int ShowFolderInOSWindow(ExtensionString pathname)
{
    GError *error;
    gtk_show_uri(NULL, pathname.c_str(), GDK_CURRENT_TIME, &error);
    
    if(error != NULL)
        g_warning ("%s %s", "Error launching preview", error->message);
    
    g_error_free(error);

    return NO_ERROR;
}

int ConvertLinuxErrorCode(int errorCode, bool isReading)
{
    // switch (errorCode) {
    // case NO_ERROR:
    //     return NO_ERROR;
    // case ENOENT:
    // case ERROR_FILE_NOT_FOUND:
    //     return ERR_NOT_FOUND;
    // case EACCESS:
    //     return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
    // case ERROR_WRITE_PROTECT:
    //     return ERR_CANT_WRITE;
    // case ERROR_HANDLE_DISK_FULL:
    //     return ERR_OUT_OF_SPACE;
    // case ERROR_ALREADY_EXISTS:
    //     return ERR_FILE_EXISTS;
    // default:
    //     return ERR_UNKNOWN;
    // }
    return errorCode;
}