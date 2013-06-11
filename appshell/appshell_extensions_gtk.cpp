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

#include <gtk/gtk.h>
#include "appshell_extensions.h"
#include "appshell_extensions_platform.h"
#include "client_handler.h"

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <X11/Xlib.h>

GtkWidget* _menuWidget;

int ConvertLinuxErrorCode(int errorCode, bool isReading = true);

extern bool isReallyClosing;

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

void CloseLiveBrowser(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    system("killall -9 google-chrome &");
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
    GError* error = NULL;
    gtk_show_uri(NULL, url.c_str(), GDK_CURRENT_TIME, &error);
    g_error_free(error);
    return NO_ERROR;
}

int32 IsNetworkDrive(ExtensionString path, bool& isRemote)
{
}

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
    static int mkdirError = NO_ERROR;
    mode = mode | 0777;     //#TODO make Brackets set mode != 0

    struct stat buf;
    if((stat(path.substr(0, path.find_last_of('/')).c_str(), &buf) < 0) && errno==ENOENT)
        MakeDir(path.substr(0, path.find_last_of('/')), mode);

    if(mkdirError != 0)
        return mkdirError;

    if(mkdir(path.c_str(),mode)==-1)
        mkdirError = ConvertLinuxErrorCode(errno);
    return NO_ERROR;
}

int Rename(ExtensionString oldName, ExtensionString newName)
{
    if (rename(oldName.c_str(), newName.c_str())==-1)
        return ConvertLinuxErrorCode(errno);
}

int GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir)
{
    struct stat buf;
    if(stat(filename.c_str(),&buf)==-1)
        return ConvertLinuxErrorCode(errno);

    modtime = buf.st_mtime;
    isDir = S_ISDIR(buf.st_mode);

    return NO_ERROR;
}

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
        isReallyClosing = true;
        // //# Hack because CEF's CloseBrowser() is bad. Should emit delete_event instead of directly destroying widget
        // GtkWidget* hwnd = gtk_widget_get_toplevel (browser->GetHost()->GetWindowHandle() );
        // if(gtk_widget_is_toplevel (hwnd))
        //     gtk_signal_emit_by_name(GTK_OBJECT(hwnd), "delete_event");
        // else
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
//    printf("LINUX ERROR! %d %s\n", errorCode, strerror(errorCode));
    switch (errorCode) {
    case NO_ERROR:
        return NO_ERROR;
    case ENOENT:
        return ERR_NOT_FOUND;
    case EACCES:
        return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
    case ENOTDIR:
        return ERR_NOT_DIRECTORY;
//    case ERROR_WRITE_PROTECT:
//        return ERR_CANT_WRITE;
//    case ERROR_HANDLE_DISK_FULL:
//        return ERR_OUT_OF_SPACE;
//    case ERROR_ALREADY_EXISTS:
//        return ERR_FILE_EXISTS;
    default:
        return ERR_UNKNOWN;
    }
}


int32 GetPendingFilesToOpen(ExtensionString& files)
{
}

GtkWidget* GetMenuBar(CefRefPtr<CefBrowser> browser)
{
    GtkWidget* window = (GtkWidget*)getMenuParent(browser);
    GtkWidget* widget;
    GList *children, *iter;

    children = gtk_container_get_children(GTK_CONTAINER(window));
    for(iter = children; iter != NULL; iter = g_list_next(iter)) {
        widget = (GtkWidget*)iter->data;

        if (GTK_IS_CONTAINER(widget))
            return widget;
    }

    return NULL;
}

int32 AddMenu(CefRefPtr<CefBrowser> browser, ExtensionString title, ExtensionString command,
              ExtensionString position, ExtensionString relativeId)
{
    // if (tag == kTagNotFound) {
    //     tag = NativeMenuModel::getInstance(getMenuParent(browser)).getOrCreateTag(command, ExtensionString());
    // } else {
    //     // menu already there
    //     return NO_ERROR;
    // }

    GtkWidget* menuBar = GetMenuBar(browser);
    GtkWidget* menuWidget = gtk_menu_new();
    GtkWidget* menuHeader = gtk_menu_item_new_with_label(title.c_str());
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuHeader), menuWidget);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), menuHeader);

    // FIXME add lookup for menu widgets
    _menuWidget = menuWidget;
    
    return NO_ERROR;
}

void FakeCallback() {
}

int32 AddMenuItem(CefRefPtr<CefBrowser> browser, ExtensionString parentCommand, ExtensionString itemTitle,
                  ExtensionString command, ExtensionString key, ExtensionString displayStr,
                  ExtensionString position, ExtensionString relativeId)
{
    GtkWidget* entry = gtk_menu_item_new_with_label(itemTitle.c_str());
    g_signal_connect(entry, "activate", FakeCallback, NULL);
    // FIXME add lookup for menu widgets
    gtk_menu_shell_append(GTK_MENU_SHELL(_menuWidget), entry);

    return NO_ERROR;
}

int32 RemoveMenu(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId)
{
    return NO_ERROR;
}

int32 RemoveMenuItem(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId)
{
    return NO_ERROR;
}

int32 GetMenuItemState(CefRefPtr<CefBrowser> browser, ExtensionString commandId, bool& enabled, bool& checked, int& index)
{
    return NO_ERROR;
}

int32 SetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString menuTitle)
{
    return NO_ERROR;
}

int32 GetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString& menuTitle)
{
    return NO_ERROR;
}

int32 SetMenuItemShortcut(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString shortcut, ExtensionString displayStr)
{
    return NO_ERROR;
}

int32 GetMenuPosition(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId, ExtensionString& parentId, int& index)
{
    return NO_ERROR;
}
