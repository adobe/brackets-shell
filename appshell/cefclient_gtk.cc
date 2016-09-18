// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "cefclient.h"
#include "include/cef_app.h"
#include "include/cef_version.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "include/wrapper/cef_helpers.h"
#include "client_handler.h"
#include "appshell/browser/client_app_browser.h"
#include "appshell/common/client_app_other.h"
#include "appshell/common/client_switches.h"
#include "appshell/renderer/client_app_renderer.h"
#include "appshell_node_process.h"
#include "appshell_helpers.h"

static std::string APPICONS[] = {"appshell32.png","appshell48.png","appshell128.png","appshell256.png"};
char szWorkingDir[512];  // The current working directory
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

// Global functions

std::string AppGetWorkingDirectory() {
  return szWorkingDir;
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

namespace client {
namespace {

int RunMain(int argc, char* argv[]) {
  // Create a copy of |argv| on Linux because Chromium mangles the value
  // internally (see issue #620).
  CefScopedArgArray scoped_arg_array(argc, argv);
  char** argv_copy = scoped_arg_array.array();

  CefMainArgs main_args(argc, argv);

  g_appStartupTime = time(NULL);

  // Parse command-line arguments.
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromArgv(argc, argv);

  // Create a ClientApp of the correct type.
  CefRefPtr<CefApp> app;
  ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);
  if (process_type == ClientApp::BrowserProcess) {
    app = new ClientAppBrowser();
  } else if (process_type == ClientApp::RendererProcess ||
             process_type == ClientApp::ZygoteProcess) {
    // On Linux the zygote process is used to spawn other process types. Since
    // we don't know what type of process it will be give it the renderer
    // client.
    app = new ClientAppRenderer();
  } else if (process_type == ClientApp::OtherProcess) {
    app = new ClientAppOther();
  }

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app, NULL);
  if (exit_code >= 0)
    return exit_code;

  gtk_init(&argc, &argv);

  //Retrieve the current working directory
  if (!getcwd(szWorkingDir, sizeof (szWorkingDir)))
    return -1;

  GtkWidget* window;

  CefSettings settings;

/*
  // Populate the settings based on command line arguments.
  AppGetSettings(settings, app);

  settings.no_sandbox = TRUE;

  // Check cache_path setting
  if (CefString(&settings.cache_path).length() == 0) {
    CefString(&settings.cache_path) = AppGetCachePath();
  }
*/

  if (appshell::AppInitInitialUrl(command_line) < 0) {
    return 0;
  }

  std::string szInitialUrl = appshell::AppGetInitialURL();

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

  // Necessary to enable document.executeCommand("paste")
  browserSettings.javascript_access_clipboard = STATE_ENABLED;
  browserSettings.javascript_dom_paste = STATE_ENABLED;

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

}  // namespace
}  // namespace client


// Program entry point function.
int main(int argc, char* argv[]) {
  return client::RunMain(argc, argv);
}
