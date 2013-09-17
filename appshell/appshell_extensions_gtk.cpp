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
#include <unistd.h>
#include <X11/Xlib.h>

GtkWidget* _menuWidget;

// Supported browsers (order matters):
//   - google-chorme 
//   - chromium-browser - chromium executable name (in ubuntu)
//   - chromium - other chromium executable name (in arch linux)
std::string browsers[3] = {"google-chrome", "chromium-browser", "chromium"};

int ConvertLinuxErrorCode(int errorCode, bool isReading = true);
int ConvertGnomeErrorCode(GError* gerror, bool isReading = true);

extern bool isReallyClosing;

int GErrorToErrorCode(GError *gerror) {
    int error = ConvertGnomeErrorCode(gerror);
    
    // uncomment to see errors printed to the console
    //g_warning(gerror->message);
    g_error_free(gerror);
    
    return error;
}

int32 OpenLiveBrowser(ExtensionString argURL, bool enableRemoteDebugging)
{
    const char *launch = "%s --allow-file-access-from-files %s %s";
    gchar *remoteDebugging = "";
    gchar *profilePath = "%s\\live-dev-profile";
    gchar *cmdline;
    int error = NO_ERROR;
    GError *gerror = NULL;
    
    if (enableRemoteDebugging) {
        profilePath = g_strdup_printf(remoteDebugging, ClientApp::AppGetSupportDirectory().c_str());
        remoteDebugging = "--user-data-dir=%s --no-first-run --no-default-browser-check --remote-debugging-port=9222";
        remoteDebugging = g_strdup_printf(remoteDebugging, profilePath);
    }

    // check for supported browsers (in PATH directories)
    for (size_t i = 0; i < sizeof(browsers) / sizeof(browsers[0]); i++) {
        cmdline = g_strdup_printf(launch, browsers[i].c_str(), argURL.c_str(), remoteDebugging);

        if (g_spawn_command_line_async(cmdline, &gerror)) {
            // browser is found in os; stop iterating
            error = NO_ERROR;
            break;
        } else {
            error = ConvertGnomeErrorCode(gerror);
            g_error_free(gerror);
        }

        g_free(cmdline);
    }
    
    g_free(profilePath);
    g_free(remoteDebugging);

    return error;
}

void CloseLiveBrowser(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    const char *killall = "killall -9 %s";
    gchar *cmdline;
    gint exitstatus;
    GError *gerror = NULL;
    int error = NO_ERROR;
    CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();

    // check for supported browsers (in PATH directories)
    for (size_t i = 0; i < sizeof(browsers) / sizeof(browsers[0]); i++) {
        cmdline = g_strdup_printf(killall, browsers[i].c_str());

        // FIXME (jasonsanjose): use async
        if (!g_spawn_command_line_sync(cmdline, NULL, NULL, &exitstatus, &gerror)) {
            error = ConvertGnomeErrorCode(gerror);
            g_error_free(gerror);
        }

        g_free(cmdline);

        // browser is found in os; stop iterating
        if (exitstatus == 0) {
            error = NO_ERROR;
            break;
        }
    }

    responseArgs->SetInt(1, error);
    browser->SendProcessMessage(PID_RENDERER, response);
}

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

int32 ShowSaveDialog(ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString proposedNewFilename,
                     ExtensionString& newFilePath)
{
    GtkWidget *openSaveDialog;
    
    openSaveDialog = gtk_file_chooser_dialog_new(title.c_str(),
        NULL,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (openSaveDialog), TRUE);	
    if (!initialDirectory.empty())
    {
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (openSaveDialog), proposedNewFilename.c_str());
        
        ExtensionString folderURI = std::string("file:///") + initialDirectory;
        gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (openSaveDialog), folderURI.c_str());
    }
    
    if (gtk_dialog_run (GTK_DIALOG (openSaveDialog)) == GTK_RESPONSE_ACCEPT)
    {
        char* filePath;
        filePath = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (openSaveDialog));
        newFilePath = filePath;
        g_free (filePath);
    }
    
    gtk_widget_destroy (openSaveDialog);
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
    if (encoding != "utf8")
        return NO_ERROR;    //#TODO ERR_UNSUPPORTED_ENCODING

    int error = NO_ERROR;
    GError *gerror = NULL;
    gchar *file_get_contents = NULL;
    gsize len = 0;
    
    if (!g_file_get_contents(filename.c_str(), &file_get_contents, &len, &gerror)) {
        error = GErrorToErrorCode(gerror);
    }

    contents.assign(file_get_contents, len);
    
    g_free(file_get_contents);

    return error;
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



void MoveFileOrDirectoryToTrash(ExtensionString filename, CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    int error = NO_ERROR;
    GFile *file = g_file_new_for_path(filename.c_str());
    GError *gerror = NULL;
    if (!g_file_trash(file, NULL, &gerror)) {
        error = ConvertGnomeErrorCode(gerror);
        g_error_free(gerror);
    }
    g_object_unref(file);
    
    response->GetArgumentList()->SetInt(1, error);
    browser->SendProcessMessage(PID_RENDERER, response);
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
            browser->GetHost()->CloseBrowser(true);
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
    int error = NO_ERROR;
    GError *gerror = NULL;
    gchar *uri = g_strdup_printf("file://%s", pathname.c_str());
    
    if (!gtk_show_uri(NULL, uri, GDK_CURRENT_TIME, &gerror)) {
        error = ConvertGnomeErrorCode(gerror);
        g_warning(gerror->message);
        g_error_free(gerror);
    }
    
    g_free(uri);

    return error;
}


int ConvertGnomeErrorCode(GError* gerror, bool isReading) 
{
    if (gerror == NULL) 
        return NO_ERROR;

    if (gerror->domain == G_FILE_ERROR) {
        switch(gerror->code) {
        case G_FILE_ERROR_EXIST:
            return ERR_FILE_EXISTS;
        case G_FILE_ERROR_NOTDIR:
            return ERR_NOT_DIRECTORY;
        case G_FILE_ERROR_ISDIR:
            return ERR_NOT_FILE;
        case G_FILE_ERROR_NXIO:
        case G_FILE_ERROR_NOENT:
            return ERR_NOT_FOUND;
        case G_FILE_ERROR_NOSPC:
            return ERR_OUT_OF_SPACE;
        case G_FILE_ERROR_INVAL:
            return ERR_INVALID_PARAMS;
        case G_FILE_ERROR_ROFS:
            return ERR_CANT_WRITE;
        case G_FILE_ERROR_BADF:
        case G_FILE_ERROR_ACCES:
        case G_FILE_ERROR_PERM:
        case G_FILE_ERROR_IO:
           return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
        default:
            return ERR_UNKNOWN;
        }  
    } else if (gerror->domain == G_IO_ERROR) {
        switch(gerror->code) {
        case G_IO_ERROR_NOT_FOUND:
            return ERR_NOT_FOUND;
        case G_IO_ERROR_NO_SPACE:
            return ERR_OUT_OF_SPACE;
        case G_IO_ERROR_INVALID_ARGUMENT:
            return ERR_INVALID_PARAMS;
        default:
            return ERR_UNKNOWN;
        }
    }
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

int32 CopyFile(ExtensionString src, ExtensionString dest)
{
    int error = NO_ERROR;
    GFile *source = g_file_new_for_path(src.c_str());
    GFile *destination = g_file_new_for_path(dest.c_str());
    GError *gerror = NULL;

    if (!g_file_copy(source, destination, (GFileCopyFlags)(G_FILE_COPY_OVERWRITE|G_FILE_COPY_NOFOLLOW_SYMLINKS|G_FILE_COPY_TARGET_DEFAULT_PERMS), NULL, NULL, NULL, &gerror)) {
        error = ConvertGnomeErrorCode(gerror);
        g_error_free(gerror);
    }
    g_object_unref(source);
    g_object_unref(destination);

    return error;    
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

void DragWindow(CefRefPtr<CefBrowser> browser)
{
    // TODO
}

