// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include "cefclient.h"
#include "include/cef_app.h"
#include "include/cef_version.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "client_handler.h"
#include "client_switches.h"
#include "appshell_node_process.h"

static std::string APPICONS[] = {"appshell32.png","appshell48.png","appshell128.png","appshell256.png"};
char szWorkingDir[512];  // The current working directory
std::string szInitialUrl;
std::string szRunningDir;
int add_handler_id;
bool isReallyClosing = false;

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

// Application startup time
time_t g_appStartupTime;

void destroy(void) {
  CefQuitMessageLoop();
}

void TerminationSignalHandler(int signatl) {
  destroy();
}
void HandleAdd(GtkContainer *container,
               GtkWidget *widget,
               gpointer user_data) {
  g_signal_handler_disconnect(container, add_handler_id);
  if(gtk_widget_get_can_focus(widget)) {
    gtk_widget_grab_focus(widget);
  }
  else {
    add_handler_id = g_signal_connect(G_OBJECT(widget), "add",
                                      G_CALLBACK(HandleAdd), NULL);
  }
}

static gboolean HandleQuit(int signatl) {
  if (!isReallyClosing && g_handler.get() && g_handler->GetBrowserId()) {
    CefRefPtr<CommandCallback> callback = new CloseWindowCommandCallback(g_handler->GetBrowser());
    
    g_handler->SendJSCommand(g_handler->GetBrowser(), FILE_CLOSE_WINDOW, callback);
    return TRUE;
  }
  destroy();
}

bool FileExists(std::string path) {
  struct stat buf;
  return (stat(path.c_str(), &buf) >= 0) && (S_ISREG(buf.st_mode));
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
        szInitialUrl = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
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

GtkWidget* AddMenuEntry(GtkWidget* menu_widget, const char* text,
                        GCallback callback) {
  GtkWidget* entry = gtk_menu_item_new_with_label(text);
  g_signal_connect(entry, "activate", callback, NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_widget), entry);
  return entry;
}

GtkWidget* CreateMenu(GtkWidget* menu_bar, const char* text) {
  GtkWidget* menu_widget = gtk_menu_new();
  GtkWidget* menu_header = gtk_menu_item_new_with_label(text);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_header), menu_widget);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_header);
  return menu_widget;
}

// Callback for Debug > Get Source... menu item.
gboolean GetSourceActivated(GtkWidget* widget) {
  return FALSE;
}

int main(int argc, char* argv[]) {
  CefMainArgs main_args(argc, argv);

  g_appStartupTime = time(NULL);

  gtk_init(&argc, &argv);
  CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app.get(), NULL);
  if (exit_code >= 0)
    return exit_code;

  //Retrieve the current working directory
  if (!getcwd(szWorkingDir, sizeof (szWorkingDir)))
    return -1;

  GtkWidget* window;

  // Parse command line arguments.
  AppInitCommandLine(argc, argv);

  CefSettings settings;

  // Populate the settings based on command line arguments.
  AppGetSettings(settings, app);

  settings.no_sandbox = TRUE;

  // Check cache_path setting
  if (CefString(&settings.cache_path).length() == 0) {
    CefString(&settings.cache_path) = AppGetCachePath();
  }
  
  CefRefPtr<CefCommandLine> cmdLine = AppGetCommandLine();
  
  if (cmdLine->HasSwitch(cefclient::kStartupPath)) {
    szInitialUrl = cmdLine->GetSwitchValue(cefclient::kStartupPath);
  } else {
    szInitialUrl = AppGetRunningDirectory();
    szInitialUrl.append("/dev/src/index.html");
  
    if (!FileExists(szInitialUrl)) {
      szInitialUrl = AppGetRunningDirectory();
      szInitialUrl.append("/www/index.html");
  
      if (!FileExists(szInitialUrl)) {
        if (GetInitialUrl() < 0)
          return 0;
      }
    }
  }

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get(), NULL);
  
  // Set window icon
  std::vector<std::string> icons(APPICONS, APPICONS + sizeof(APPICONS) / sizeof(APPICONS[0]) );
  GList *list = NULL;
  for (int i = 0; i < icons.size(); ++i) {
    std::string path = icons[i];
    
    GdkPixbuf *icon = gdk_pixbuf_new_from_file(path.c_str(), NULL);
    if (!icon)
       continue;
    
    list = g_list_append(list, icon);
  }

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  gtk_window_set_icon_list(GTK_WINDOW(window), list);
  
  // Free icon list
  g_list_foreach(list, (GFunc) g_object_unref, NULL);
  g_list_free(list);

  GtkWidget* vbox = gtk_vbox_new(FALSE, 0);

  GtkWidget* menuBar = gtk_menu_bar_new();
  // GtkWidget* debug_menu = CreateMenu(menuBar, "Tests");
  // AddMenuEntry(debug_menu, "Hello World Menu",
  //              G_CALLBACK(GetSourceActivated));

  gtk_box_pack_start(GTK_BOX(vbox), menuBar, FALSE, FALSE, 0);

  g_signal_connect(G_OBJECT(window), "delete_event",
                   G_CALLBACK(HandleQuit), NULL);
  g_signal_connect(G_OBJECT(window), "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &window);
  add_handler_id = g_signal_connect(G_OBJECT(window), "add",
                                      G_CALLBACK(HandleAdd), NULL);
  // g_signal_connect(G_OBJECT(window), "destroy",
  //                  G_CALLBACK(destroy), NULL);

  // Create the handler.
  g_handler = new ClientHandler();
  g_handler->SetMainHwnd(vbox);

  // Create the browser view.
  CefWindowInfo window_info;
  CefBrowserSettings browserSettings;

  browserSettings.web_security = STATE_DISABLED;

  window_info.SetAsChild(vbox);

  CefBrowserHost::CreateBrowser(
      window_info,
      static_cast<CefRefPtr<CefClient> >(g_handler),
      "file://"+szInitialUrl, browserSettings, NULL);

  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show_all(GTK_WIDGET(window));

  // Install an signal handler so we clean up after ourselves.
  signal(SIGINT, TerminationSignalHandler);
  signal(SIGTERM, TerminationSignalHandler);
    
  // Start the node server process
  startNodeProcess();

  CefRunMessageLoop();

  CefShutdown();

  return 0;
}

CefString AppGetProductVersionString() {
  // TODO
  return CefString("");
}

CefString AppGetChromiumVersionString() {
  // TODO
  return CefString("");
}
