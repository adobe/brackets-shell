// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <string>
#include "client_handler.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/wrapper/cef_helpers.h"

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

void ClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
#ifdef SHOW_TOOLBAR_UI
  CEF_REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
      // Set the edit window text
    std::string urlStr(url);
    gtk_entry_set_text(GTK_ENTRY(m_EditHwnd), urlStr.c_str());
  }
#endif // SHOW_TOOLBAR_UI
}

void ClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  CEF_REQUIRE_UI_THREAD();

  GtkWidget* window = gtk_widget_get_ancestor(
      GTK_WIDGET(browser->GetHost()->GetWindowHandle()),
      GTK_TYPE_WINDOW);
  std::string titleStr(title);
  gtk_window_set_title(GTK_WINDOW(window), titleStr.c_str());
}

void ClientHandler::SendNotification(NotificationType type) {
  // TODO(port): Implement this method.
}

void ClientHandler::SetNavState(bool canGoBack, bool canGoForward) {
}

void ClientHandler::CloseMainWindow() {
  // TODO(port): Check if this is enough
  gtk_main_quit();
}

void ClientHandler::ComputePopupPlacement(CefWindowInfo& windowInfo)
{
  // TODO Finish this thing
}

void ClientHandler::PopupCreated(CefRefPtr<CefBrowser> browser)
{
    // TODO Finish this thing
    browser->GetHost()->SetFocus(true);
}

//#TODO Implement window handling, e.g.: CloseWindowCallback

bool ClientHandler::CanCloseBrowser(CefRefPtr<CefBrowser> browser) {
	return true;
}

bool ClientHandler::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                                  const CefKeyEvent& event,
                                  CefEventHandle os_event,
                                  bool* is_keyboard_shortcut) {
    
    // TODO
    return false;
}

bool ClientHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                               const CefKeyEvent& event,
                               CefEventHandle os_event) {
    // TODO
    return false;
}
