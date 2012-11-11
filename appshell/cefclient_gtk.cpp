// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include "cefclient.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "client_handler.h"
#include "gtk/appicon.h"

char szWorkingDir[512];  // The current working directory
std:: string szInitialUrl;
std:: string szRunningDir;
bool isReallyClosing = false;

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

//Application startup time
time_t g_appStartupTime;

void destroy(void) {
  CefQuitMessageLoop();
}

void TerminationSignalHandler(int signatl) {
  destroy();
}

static gboolean HandleQuit(int signatl) {
  if (!isReallyClosing && g_handler.get() && g_handler->GetBrowserId()) {
    CefRefPtr<CommandCallback> callback = new CloseWindowCommandCallback(g_handler->GetBrowser());
    
    g_handler->SendJSCommand(g_handler->GetBrowser(), FILE_CLOSE_WINDOW, callback);
    return TRUE;
  }
  destroy();
}

int GetInitialUrl() {
  GtkWidget *dialog;
     const char* dialog_title = "Please select the index.html file";
     GtkFileChooserAction file_or_directory = GTK_FILE_CHOOSER_ACTION_OPEN ;
     dialog = gtk_file_chooser_dialog_new (dialog_title,
                          NULL,
                          file_or_directory,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                          NULL);
     
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
      {
        szInitialUrl.append(gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog)));
        gtk_widget_destroy (dialog);
        return 0;
      }
    return -1;
}

// Global functions

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

CefString AppGetCachePath() {
  std::string cachePath = std::string(ClientApp::AppGetSupportDirectory()) + "/cef_data";

  return CefString(cachePath);
}

int main(int argc, char* argv[]) {
  CefMainArgs main_args(argc, argv);

  g_appStartupTime = time(NULL);

  CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app.get());
  if (exit_code >= 0)
    return exit_code;

  //Retrieve the current working directory
  if (!getcwd(szWorkingDir, sizeof (szWorkingDir)))
    return -1;

  GtkWidget* window;

  gtk_init(&argc, &argv);

  // Parse command line arguments.
  AppInitCommandLine(argc, argv);

  CefSettings settings;

  // Populate the settings based on command line arguments.
  AppGetSettings(settings, app);

  // Check cache_path setting
  if (CefString(&settings.cache_path).length() == 0) {
    CefString(&settings.cache_path) = AppGetCachePath();
    printf("No cache_path supplied by default\n");
  }

  szInitialUrl = AppGetRunningDirectory();
  szInitialUrl.append("/www/index.html");

  {
    struct stat buf;
    if(!(stat(szInitialUrl.c_str(), &buf) >= 0) || !(S_ISREG(buf.st_mode)))
      if(GetInitialUrl() < 0)
        return 0;
  }

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get());

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  gtk_window_set_icon(GTK_WINDOW(window), gdk_pixbuf_new_from_inline(-1, appicon, FALSE, NULL));

  GtkWidget* vbox = gtk_vbox_new(FALSE, 0);

#ifdef SHOW_TOOLBAR_UI
  GtkToolItem* back = gtk_tool_button_new(NULL, NULL);
  GtkToolItem* forward = gtk_tool_button_new(NULL, NULL);
  GtkToolItem* reload = gtk_tool_button_new(NULL, NULL);
  GtkToolItem* stop = gtk_tool_button_new(NULL, NULL);

  GtkWidget* m_editWnd = gtk_entry_new();
#endif // SHOW_TOOLBAR_UI

  g_signal_connect(G_OBJECT(window), "delete_event",
                   G_CALLBACK(HandleQuit), NULL);
  g_signal_connect(G_OBJECT(window), "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &window);
  // g_signal_connect(G_OBJECT(window), "destroy",
  //                  G_CALLBACK(destroy), NULL);

  // Create the handler.
  g_handler = new ClientHandler();
  g_handler->SetMainHwnd(vbox);
#ifdef SHOW_TOOLBAR_UI
  g_handler->SetEditHwnd(m_editWnd);
  g_handler->SetButtonHwnds(GTK_WIDGET(back), GTK_WIDGET(forward),
                            GTK_WIDGET(reload), GTK_WIDGET(stop));
#endif // SHOW_TOOLBAR_UI

  // Create the browser view.
  CefWindowInfo window_info;
  CefBrowserSettings browserSettings;

  // Populate the settings based on command line arguments.
  AppGetBrowserSettings(browserSettings);

  browserSettings.file_access_from_file_urls_allowed = true;
  browserSettings.universal_access_from_file_urls_allowed = true;

  window_info.SetAsChild(vbox);

  CefBrowserHost::CreateBrowser(
      window_info,
      static_cast<CefRefPtr<CefClient> >(g_handler),
      "file://"+szInitialUrl, browserSettings);

  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show_all(GTK_WIDGET(window));

  // Install an signal handler so we clean up after ourselves.
  signal(SIGINT, TerminationSignalHandler);
  signal(SIGTERM, TerminationSignalHandler);

  CefRunMessageLoop();

  CefShutdown();

  return 0;
}

