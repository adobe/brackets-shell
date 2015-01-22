// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <string>
#include <gtk-2.0/gtk/gtksignal.h>
#include <glib-2.0/gobject/gsignal.h>
#include "client_handler.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_app.h"

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

extern bool isReallyClosing;

void ClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
#ifdef SHOW_TOOLBAR_UI
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
      // Set the edit window text
    std::string urlStr(url);
    gtk_entry_set_text(GTK_ENTRY(m_EditHwnd), urlStr.c_str());
  }
#endif // SHOW_TOOLBAR_UI
}

void ClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  REQUIRE_UI_THREAD();
  std::string titleStr(title);

  // Retrieve the X11 display shared with Chromium.
  ::Display* display = cef_get_xdisplay();
  DCHECK(display);

  // Retrieve the X11 window handle for the browser.
  ::Window window = browser->GetHost()->GetWindowHandle();
  DCHECK(window != kNullWindowHandle);

  // Retrieve the atoms required by the below XChangeProperty call.
  const char* kAtoms[] = {
    "_NET_WM_NAME",
    "UTF8_STRING"
  };
  Atom atoms[2];
  int result = XInternAtoms(display, const_cast<char**>(kAtoms), 2, false,
                            atoms);
  if (!result)
    NOTREACHED();

  // Set the window title.
  XChangeProperty(display,
                  window,
                  atoms[0],
                  atoms[1],
                  8,
                  PropModeReplace,
                  reinterpret_cast<const unsigned char*>(titleStr.c_str()),
                  titleStr.size());

  // TODO(erg): This is technically wrong. So XStoreName and friends expect
  // this in Host Portable Character Encoding instead of UTF-8, which I believe
  // is Compound Text. This shouldn't matter 90% of the time since this is the
  // fallback to the UTF8 property above.
  XStoreName(display, browser->GetHost()->GetWindowHandle(), titleStr.c_str());
}

void ClientHandler::SendNotification(NotificationType type) {
  // TODO(port): Implement this method.
}

void ClientHandler::SetLoading(bool isLoading) {
#ifdef SHOW_TOOLBAR_UI
  if (isLoading)
    gtk_widget_set_sensitive(GTK_WIDGET(m_StopHwnd), true);
  else
    gtk_widget_set_sensitive(GTK_WIDGET(m_StopHwnd), false);
#endif // SHOW_TOOLBAR_UI
}

void ClientHandler::SetNavState(bool canGoBack, bool canGoForward) {
#ifdef SHOW_TOOLBAR_UI
  if (canGoBack)
    gtk_widget_set_sensitive(GTK_WIDGET(m_BackHwnd), true);
  else
    gtk_widget_set_sensitive(GTK_WIDGET(m_BackHwnd), false);

  if (canGoForward)
    gtk_widget_set_sensitive(GTK_WIDGET(m_ForwardHwnd), true);
  else
    gtk_widget_set_sensitive(GTK_WIDGET(m_ForwardHwnd), false);
#endif // SHOW_TOOLBAR_UI
}

void ClientHandler::AfterClose() {
  gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(g_handler->GetMainHwnd())));
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
