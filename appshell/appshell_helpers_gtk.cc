/*
 * Copyright (c) 2016 - present Adobe Systems Incorporated. All rights reserved.
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

#include <sys/stat.h>

#include "include/cef_version.h"
#include "appshell/common/client_switches.h"
#include "config.h"

namespace appshell {

static std::string szInitialUrl;
static std::string szRunningDir;

CefString AppGetProductVersionString() {
    // TODO
    return CefString("");
}

CefString AppGetChromiumVersionString() {
    // TODO
    return CefString("");
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

std::string AppGetRunningDirectory() {
  if(szRunningDir.length() > 0)
    return szRunningDir;

  char buf[512];
  int len = readlink("/proc/self/exe", buf, 512);

//  if(len < 0)
//    return AppGetWorkingDirectory();  //# Well, can't think of any real-world case where this would be happen

  for(; len >= 0; len--){
    if(buf[len] == '/'){
      buf[len] = '\0';
      szRunningDir.append(buf);
      return szRunningDir;
    }
  }
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

bool FileExists(std::string path) {
    struct stat buf;
    return (stat(path.c_str(), &buf) >= 0) && (S_ISREG(buf.st_mode));
}

int AppInitInitialUrl(CefRefPtr<CefCommandLine> command_line) {
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

CefString AppGetInitialURL() {
    return szInitialUrl;
}

}  // namespace appshell
