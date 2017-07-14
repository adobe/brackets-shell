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
//#include "include/cef_runnable.h"
#include "client_handler.h"
#include "appshell/common/client_switches.h"
#include "appshell/appshell_helpers.h"
#include "appshell_node_process.h"
#include "appshell/common/client_app.h"
#include "appshell/common/client_app_other.h"
#include "appshell/browser/client_app_browser.h"
#include "appshell/renderer/client_app_renderer.h"
#include "appshell/browser/main_context_impl.h"
#include "appshell/browser/main_message_loop_std.h"

static std::string APPICONS[] = {"appshell32.png","appshell48.png","appshell128.png","appshell256.png"};
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



  //CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the secondary process, if any.
  //int exit_code = CefExecuteProcess(main_args, app.get(), NULL);
  //if (exit_code >= 0)
  //    return exit_code;

// Create a ClientApp of the correct type.
  
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromArgv(argc, argv);
  CefRefPtr<CefApp> app;
  client::ClientApp::ProcessType process_type = client::ClientApp::GetProcessType(command_line);
  if (process_type == client::ClientApp::BrowserProcess) {
    app = new client::ClientAppBrowser();
  } else if (process_type == client::ClientApp::RendererProcess ||
             process_type == client::ClientApp::ZygoteProcess) {
    // On Linux the zygote process is used to spawn other process types. Since
    // we don't know what type of process it will be give it the renderer
    // client.
    app = new client::ClientAppRenderer();
  } else if (process_type == client::ClientApp::OtherProcess) {
    app = new client::ClientAppOther();
  }

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app, NULL);
  if (exit_code >= 0)
    return exit_code; 
  
  scoped_ptr<client::MainContextImpl> context(new client::MainContextImpl(command_line, true));
  
  CefSettings settings2;
  settings2.no_sandbox = TRUE;

  // Populate the settings based on command line arguments.
  context->PopulateSettings(&settings2);

  // Create the main message loop object.
  scoped_ptr<client::MainMessageLoop> message_loop(new client::MainMessageLoopStd);

  // Initialize CEF.
  context->Initialize(main_args, settings2, app, NULL);

  //Retrieve the current working directory
  if (!appshell::AppInitWorkingDirectory())
    return -1;

  GtkWidget* window;

  // Parse command line arguments.browser
  CefRefPtr<CefCommandLine> cmdLine = CefCommandLine::CreateCommandLine();
  cmdLine->InitFromArgv(argc, argv);

  CefSettings settings;

  // Populate the settings based on command line arguments.
  AppGetSettings(settings, cmdLine);

  settings.no_sandbox = TRUE;

  // Check cache_path setting
  if (CefString(&settings.cache_path).length() == 0) {
    CefString(&settings.cache_path) = appshell::AppGetCachePath();
  }

  if (appshell::AppInitInitialURL(cmdLine) < 0) {
    return 0;
  }

  // Initialize CEF.
  //CefInitialize(main_args, settings, app.get(), NULL);
  
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
  g_handler->SetMainHwnd((CefWindowHandle)vbox);

  // Create the browser view.
  CefWindowInfo window_info;
  CefBrowserSettings browserSettings;

  browserSettings.web_security = STATE_DISABLED;

  // Necessary to enable document.executeCommand("paste")
  browserSettings.javascript_access_clipboard = STATE_ENABLED;
  browserSettings.javascript_dom_paste = STATE_ENABLED;
  CefRect someRect;
  window_info.SetAsChild((CefWindowHandle)vbox, someRect);

  /*CefBrowserHost::CreateBrowser(
      window_info,
      static_cast<CefRefPtr<CefClient> >(g_handler),
      //"file://" + appshell::AppGetInitialURL()
      "https://www.google.com", browserSettings, NULL);*/

  //gtk_container_add(GTK_CONTAINER(window), vbox);
  //gtk_widget_show_all(GTK_WIDGET(window));

   // Create the first window.
  context->GetRootWindowManager()->CreateRootWindow(
      //!command_line->HasSwitch(switches::kHideControls),  // Show controls.
      false,
      settings.windowless_rendering_enabled ? true : false,
      CefRect(),        // Use default system size.
      std::string());   // Use default URL.

  // Install an signal handler so we clean up after ourselves.
  signal(SIGINT, TerminationSignalHandler);
  signal(SIGTERM, TerminationSignalHandler);
    
  // Start the node server process
  startNodeProcess();

  //CefRunMessageLoop();

  //CefShutdown();

// Run the message loop. This will block until Quit() is called.
  int result = message_loop->Run();

  // Shut down CEF.
  context->Shutdown();

  // Release objects in reverse order of creation.
  message_loop.reset();
  context.reset();

  return result;

  //return 0;
}
