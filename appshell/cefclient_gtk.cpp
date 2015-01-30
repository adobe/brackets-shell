// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <gdk/gdkx.h>

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

static std::string APPICONS[] = {"appshell32.png", "appshell48.png", "appshell128.png", "appshell256.png"};
char szWorkingDir[512]; // The current working directory
std::string szInitialUrl;
std::string szRunningDir;
int add_handler_id;
bool isReallyClosing = false;

static gint DEFAULT_SIZE_HEIGHT = 600;
static gint DEFAULT_SIZE_WIDTH = 800;

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

// Application startup time
time_t g_appStartupTime;

// Error handler for X11

int XErrorHandlerImpl(Display *display, XErrorEvent *event) {
  LOG(WARNING)
          << "X error received: "
          << "type " << event->type << ", "
          << "serial " << event->serial << ", "
          << "error_code " << static_cast<int> (event->error_code) << ", "
          << "request_code " << static_cast<int> (event->request_code) << ", "
          << "minor_code " << static_cast<int> (event->minor_code);
  return 0;
}

int XIOErrorHandlerImpl(Display *display) {
  return 0;
}

void destroy(void) {
  if (isReallyClosing) {
    CefQuitMessageLoop();
  }
}

void TerminationSignalHandler(int signal) {
  isReallyClosing = true;
  destroy();
}

//void HandleAdd(GtkContainer *container,
//        GtkWidget *widget,
//        gpointer user_data) {
//  g_signal_handler_disconnect(container, add_handler_id);
//  if (gtk_widget_get_can_focus(widget)) {
//    gtk_widget_grab_focus(widget);
//  } else {
//    add_handler_id = g_signal_connect(G_OBJECT(widget), "add",
//            G_CALLBACK(HandleAdd), NULL);
//  }
//}

// TODO: this is not yet called to shutdown Brackets
gboolean HandleQuit() {
  
  if (!isReallyClosing && g_handler.get()) {
    g_handler->DispatchCloseToNextBrowser();
  }
  else 
  {
    if(!g_handler.get() || g_handler->HasWindows()) {
      CefQuitMessageLoop();
      return FALSE;
    }
  }
  
  return TRUE;
    
}

bool FileExists(std::string path) {
  struct stat buf;
  return (stat(path.c_str(), &buf) >= 0) && (S_ISREG(buf.st_mode));
}

int GetInitialUrl() {
  GtkWidget *dialog;
  const char* dialog_title = "Please select the index.html file";
  GtkFileChooserAction file_or_directory = GTK_FILE_CHOOSER_ACTION_OPEN;
  dialog = gtk_file_chooser_dialog_new(dialog_title,
          NULL,
          file_or_directory,
          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
          NULL);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    szInitialUrl = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy(dialog);
    return 0;
  }
  return -1;
}

// Global functions

std::string AppGetWorkingDirectory() {
  return szWorkingDir;
}

std::string AppGetRunningDirectory() {
  if (szRunningDir.length() > 0)
    return szRunningDir;

  char buf[512];
  int len = readlink("/proc/self/exe", buf, 512);

  if (len < 0)
    return AppGetWorkingDirectory(); //# Well, can't think of any real-world case where this would be happen

  for (; len >= 0; len--) {
    if (buf[len] == '/') {
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

gboolean WindowConfigure(GtkWindow* window,
        GdkEvent* event,
        gpointer data) {
  // Called when size, position or stack order changes.
  if (g_handler) {
    CefRefPtr<CefBrowser> browser = g_handler->GetBrowser();
    if (browser) {
      // Notify the browser of move/resize events so that:
      // - Popup windows are displayed in the correct location and dismissed
      //   when the window moves.
      // - Drag&drop areas are updated accordingly.
      browser->GetHost()->NotifyMoveOrResizeStarted();
    }
  }

  return FALSE; // Don't stop this message.
}

void VboxSizeAllocated(GtkWidget* widget,
        GtkAllocation* allocation,
        void* data) {
  if (g_handler) {
    CefRefPtr<CefBrowser> browser = g_handler->GetBrowser();
    if (browser && !browser->GetHost()->IsWindowRenderingDisabled()) {
      // Size the browser window to match the GTK widget.
      ::Display* xdisplay = cef_get_xdisplay();
      ::Window xwindow = browser->GetHost()->GetWindowHandle();
      XWindowChanges changes = {0};

      // TODO: get the real value
      int g_menubar_height = 0;
      changes.width = allocation->width;
      changes.height = allocation->height - (g_menubar_height);
      changes.y = g_menubar_height;
      XConfigureWindow(xdisplay, xwindow, CWHeight | CWWidth | CWY, &changes);
    }
  }
}

gboolean WindowFocusIn(GtkWidget* widget,
        GdkEventFocus* event,
        gpointer user_data) {
  if (g_handler && event->in) {
    CefRefPtr<CefBrowser> browser = g_handler->GetBrowser();
    if (browser) {
      if (browser->GetHost()->IsWindowRenderingDisabled()) {
        // Give focus to the off-screen browser.
        browser->GetHost()->SendFocusEvent(true);
      } else {
        // Give focus to the browser window.
        browser->GetHost()->SetFocus(true);
        return TRUE;
      }
    }
  }

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

  // Parse command line arguments.
  AppInitCommandLine(argc, argv);

  // Install xlib error handlers so that the application won't be terminated
  // on non-fatal errors.
  XSetErrorHandler(XErrorHandlerImpl);
  XSetIOErrorHandler(XIOErrorHandlerImpl);

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

  // Start the node server process
  startNodeProcess();

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get(), NULL);

  // Set window icon
  std::vector<std::string> icons(APPICONS, APPICONS + sizeof (APPICONS) / sizeof (APPICONS[0]));
  GList *list = NULL;
  for (int i = 0; i < icons.size(); ++i) {
    std::string path = icons[i];

    GdkPixbuf *icon = gdk_pixbuf_new_from_file(path.c_str(), NULL);
    if (!icon)
      continue;

    list = g_list_append(list, icon);
  }

  GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), DEFAULT_SIZE_WIDTH, DEFAULT_SIZE_HEIGHT);

  gtk_window_set_icon_list(GTK_WINDOW(window), list);

  // Free icon list
  g_list_foreach(list, (GFunc) g_object_unref, NULL);
  g_list_free(list);

  GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
  g_signal_connect(vbox, "size-allocate",
          G_CALLBACK(VboxSizeAllocated), NULL);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  GtkWidget* menuBar = gtk_menu_bar_new();
  // GtkWidget* debug_menu = CreateMenu(menuBar, "Tests");
  // AddMenuEntry(debug_menu, "Hello World Menu",
  //              G_CALLBACK(GetSourceActivated));

  gtk_box_pack_start(GTK_BOX(vbox), menuBar, FALSE, FALSE, 0);

  // setup all event listener to handle focus, resizing, destruction
  g_signal_connect(G_OBJECT(window), "configure-event",
          G_CALLBACK(WindowConfigure), NULL);
  g_signal_connect(G_OBJECT(window), "focus-in-event",
          G_CALLBACK(WindowFocusIn), NULL);
  g_signal_connect(G_OBJECT(window), "delete_event",
          G_CALLBACK(HandleQuit), NULL);
  g_signal_connect(G_OBJECT(window), "destroy",
          G_CALLBACK(destroy), window);
  
//  add_handler_id = g_signal_connect(G_OBJECT(window), "add",
//          G_CALLBACK(HandleAdd), NULL);

  // Create the handler.
  g_handler = new ClientHandler();
  g_handler->SetMainHwnd(vbox);
//  g_handler->SetMainHwnd(window);

  // Create the browser view.
  CefWindowInfo window_info;
  CefBrowserSettings browserSettings;

  browserSettings.web_security = STATE_DISABLED;

  // show the window
  gtk_widget_show_all(GTK_WIDGET(window));

  window_info.SetAsChild(GDK_WINDOW_XID(gtk_widget_get_window(window)),
          CefRect(0, 0, DEFAULT_SIZE_WIDTH, DEFAULT_SIZE_HEIGHT));

  // Create the browser window
  CefBrowserHost::CreateBrowser(
          window_info,
          g_handler.get(),
          "file://" + szInitialUrl, browserSettings, NULL);

  // Install an signal handler so we clean up after ourselves.
  signal(SIGINT, TerminationSignalHandler);
  signal(SIGTERM, TerminationSignalHandler);

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
