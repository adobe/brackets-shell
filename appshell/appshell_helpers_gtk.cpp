/*
 * Copyright (c) 2012 - present Adobe Systems Incorporated. All rights reserved.
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

#include "appshell/appshell_helpers.h"

#include "appshell/browser/resource.h"
#include "appshell/common/client_switches.h"
#include "appshell/version_linux.h"
#include "include/cef_base.h"
#include "include/cef_version.h"

#include <gtk/gtk.h>
#include <pwd.h>
#include <algorithm>
//#include <MMSystem.h>
//#include <ShlObj.h>
#include <glib.h>
#include <sys/stat.h>

extern time_t g_appStartupTime;
extern char _binary_appshell_appshell_extensions_js_start;

namespace appshell {

char szWorkingDir[512];  // The current working directory
std::string szRunningDir;
std::string szInitialUrl;

CefString GetCurrentLanguage()
{
    const char* locconst =  pango_language_to_string( gtk_get_default_language() );
    //Rado: for me it prints "en-us", so I have to strip everything after the "-"
    char loc[10] = {0};
    strncpy(loc, locconst, 9);
    for(int i=0; i<8; i++)
        if ( (loc[i] == '-') || (loc[i] == '_') ) { loc[i] = 0; break; }

    //TODO Explore possibility of using locale as-is, without stripping
    return CefString(loc);
}

std::string GetExtensionJSSource()
{
    //# We objcopy the appshell/appshell_extensions.js file, and link it directly into the binary.
    //# See http://www.linuxjournal.com/content/embedding-file-executable-aka-hello-world-version-5967
    const char* p = &_binary_appshell_appshell_extensions_js_start;
    std::string content(p);
    // std::string extensionJSPath(AppGetSupportDirectory());
    // extensionJSPath.append("/appshell_extensions.js");
    // FILE* file = fopen(extensionJSPath.c_str(),"r");
    // if(file == NULL)
    // {
    //     return "";
    // }

    // fseek(file, 0, SEEK_END);
    // long int size = ftell(file);
    // rewind(file);

    // char* content = (char*)calloc(size + 1, 1);

    // fread(content,1,size,file);

    return content;
}

double GetElapsedMilliseconds()
{
    return (time(NULL) - g_appStartupTime);
}

CefString AppGetSupportDirectory()
{
    gchar *supportDir = g_strdup_printf("%s/%s", g_get_user_config_dir(), APP_NAME);
    CefString result = CefString(supportDir);
    g_free(supportDir);

    return result;
}

CefString AppGetDocumentsDirectory()
{
    const char *dir = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
    if (dir == NULL)  {
        return AppGetSupportDirectory();
    } else {
        return CefString(dir);
    }
}

CefString AppGetCachePath() {
    std::string cachePath = std::string(AppGetSupportDirectory()) + "/cef_data";

    return CefString(cachePath);
}

CefString AppGetProductVersionString() {
    std::string s = APP_NAME "/" APP_VERSION;
    CefString ret;
    ret.FromString(s);
    return ret;
}

CefString AppGetChromiumVersionString() {
    std::wostringstream versionStream(L"");
    versionStream << L"Chrome/" << cef_version_info(2) << L"." << cef_version_info(3)
                  << L"." << cef_version_info(4) << L"." << cef_version_info(5);

    return CefString(versionStream.str());
}

char* AppInitWorkingDirectory() {
    return getcwd(szWorkingDir, sizeof (szWorkingDir));
}

std::string AppGetWorkingDirectory() {
    return szWorkingDir;
}

std::string AppGetRunningDirectory() {
    if(szRunningDir.length() > 0)
        return szRunningDir;

    char buf[512];
    int len = readlink("/proc/self/exe", buf, 512);

    if(len < 0)
        return AppGetWorkingDirectory();  //# Well, can't think of any real-world case where this would be happen

    for(; len >= 0; len--){
        if(buf[len] == '/'){
            buf[len] = '\0';
            szRunningDir.append(buf);
            return szRunningDir;
        }
    }
}

bool FileExists(std::string path) {
    struct stat buf;
    return (stat(path.c_str(), &buf) >= 0) && (S_ISREG(buf.st_mode));
}

int GetInitialUrl(std::string& url) {
    GtkWidget *dialog;
    const char* dialog_title = "Please select the index.html file";
    GtkFileChooserAction file_or_directory = GTK_FILE_CHOOSER_ACTION_OPEN;
    dialog = gtk_file_chooser_dialog_new(dialog_title,
                                         NULL,
                                         file_or_directory,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        url = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        gtk_widget_destroy (dialog);
        return 0;
    }

    return -1;
}

int AppInitInitialURL(CefRefPtr<CefCommandLine> command_line) {
    if (command_line->HasSwitch(client::switches::kStartupPath)) {
        szInitialUrl = command_line->GetSwitchValue(client::switches::kStartupPath);
        return 0;
    }

    std::string url = AppGetRunningDirectory();
    url.append("/dev/src/index.html");

    if (!FileExists(url)) {
        url = AppGetRunningDirectory();
        url.append("/www/index.html");

        if (!FileExists(url)) {
            if (GetInitialUrl(url) < 0) {
                return -1;
            }
        }
    }

    szInitialUrl = url;
    return 0;
}

std::string AppGetInitialURL() {
    return szInitialUrl;
}

}  // namespace appshell
