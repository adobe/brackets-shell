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
#include "include/wrapper/cef_helpers.h"
#include "appshell/browser/client_handler_std.h"
#include <X11/Xlib.h>
#include <gdk/gdkx.h>

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

namespace client {
namespace {

int XErrorHandlerImpl(Display *display, XErrorEvent *event) {
  LOG(WARNING)
        << "X error received: "
        << "type " << event->type << ", "
        << "serial " << event->serial << ", "
        << "error_code " << static_cast<int>(event->error_code) << ", "
        << "request_code " << static_cast<int>(event->request_code) << ", "
        << "minor_code " << static_cast<int>(event->minor_code);
  return 0;
}

int XIOErrorHandlerImpl(Display *display) {
  return 0;
}

void TerminationSignalHandler(int signatl) {
  LOG(ERROR) << "Received termination signal: " << signatl;
  MainContext::Get()->GetRootWindowManager()->CloseAllWindows(true);
}

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
  if (process_type == ClientApp::RendererProcess ||
             process_type == ClientApp::ZygoteProcess || 
             process_type == ClientApp::BrowserProcess) {
    app = new ::ClientApp();
  } else if (process_type == ClientApp::OtherProcess) {
    app = new ClientAppOther();
  }

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app, NULL);
  if (exit_code >= 0)
    return exit_code;

  // Create the main context object.
  scoped_ptr<MainContextImpl> context(new MainContextImpl(command_line, true));

  CefSettings settings;

  // Populate the settings based on command line arguments.
  AppGetSettings(settings, command_line);

  // Check cache_path setting
  if (CefString(&settings.cache_path).length() == 0) {
    CefString(&settings.cache_path) = appshell::AppGetCachePath();
  }
  
  startNodeProcess();

  // Initialize CEF.
  context->Initialize(main_args, settings, app, NULL);
  
  // The Chromium sandbox requires that there only be a single thread during
  // initialization. Therefore initialize GTK after CEF.
  gtk_init(&argc, &argv_copy);

  if (appshell::AppInitInitialURL(command_line) < 0) {
    context->Shutdown();
    return 0;
  }

  // Install xlib error handlers so that the application won't be terminated
  // on non-fatal errors. Must be done after initializing GTK.
  XSetErrorHandler(XErrorHandlerImpl);
  XSetIOErrorHandler(XIOErrorHandlerImpl);

  // Install a signal handler so we clean up after ourselves.
  signal(SIGINT, TerminationSignalHandler);
  signal(SIGTERM, TerminationSignalHandler);
  
  // Create the main message loop object.
  scoped_ptr<MainMessageLoop> message_loop(new MainMessageLoopStd);

  // Create the first window.
  scoped_refptr<RootWindow> root_window = context->GetRootWindowManager()->CreateRootWindow(
      false, //Hide controls.
      settings.windowless_rendering_enabled ? true : false,
      CefRect(),        // Use default system size.
      "file://" + appshell::AppGetInitialURL()
      );
    
  // get the reference to the root window.
  GtkWidget* window = root_window->GetWindowHandle();
  if (window) {
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

    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_icon_list(GTK_WINDOW(window), list);
    
    // Free icon list
    g_list_foreach(list, (GFunc) g_object_unref, NULL);
    g_list_free(list);
  }

  // Run the message loop. This will block until Quit() is called.
  int result = message_loop->Run();

  root_window = NULL;
  
  // Shut down CEF.
  context->Shutdown();

  // Release objects in reverse order of creation.
  message_loop.reset();
  context.reset();

  return result;
}

}  // namespace
}  // namespace client


// Program entry point function.
int main(int argc, char* argv[]) {
  return client::RunMain(argc, argv);
}
