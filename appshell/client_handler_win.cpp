// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "config.h"
#include "client_handler.h"
#include <string>
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "resource.h"

bool ClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
                                  const CefPopupFeatures& popupFeatures,
                                  CefWindowInfo& windowInfo,
                                  const CefString& url,
                                  CefRefPtr<CefClient>& client,
                                  CefBrowserSettings& settings) {
  REQUIRE_UI_THREAD();

  return false;
}

void ClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  REQUIRE_UI_THREAD();

#ifdef SHOW_TOOLBAR_UI
  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain())   {
    // Set the edit window text
    SetWindowText(m_EditHwnd, std::wstring(url).c_str());
  }
#endif // SHOW_TOOLBAR_UI
}

void ClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  REQUIRE_UI_THREAD();

  // Set the frame window title bar
  CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
  if (m_BrowserId == browser->GetIdentifier())   {
    // The frame window will be the parent of the browser window
    hwnd = GetParent(hwnd);
  }
  SetWindowText(hwnd, std::wstring(title).c_str());
}

void ClientHandler::SendNotification(NotificationType type) {
  UINT id;
  switch (type) {
  case NOTIFY_CONSOLE_MESSAGE:
    id = ID_WARN_CONSOLEMESSAGE;
    break;
  case NOTIFY_DOWNLOAD_COMPLETE:
    id = ID_WARN_DOWNLOADCOMPLETE;
    break;
  case NOTIFY_DOWNLOAD_ERROR:
    id = ID_WARN_DOWNLOADERROR;
    break;
  default:
    return;
  }
  PostMessage(m_MainHwnd, WM_COMMAND, id, 0);
}

void ClientHandler::SetLoading(bool isLoading) {
#ifdef SHOW_TOOLBAR_UI
  ASSERT(m_EditHwnd != NULL && m_ReloadHwnd != NULL && m_StopHwnd != NULL);
  EnableWindow(m_EditHwnd, TRUE);
  EnableWindow(m_ReloadHwnd, !isLoading);
  EnableWindow(m_StopHwnd, isLoading);
#endif // SHOW_TOOLBAR_UI
}

void ClientHandler::SetNavState(bool canGoBack, bool canGoForward) {
#ifdef SHOW_TOOLBAR_UI
  ASSERT(m_BackHwnd != NULL && m_ForwardHwnd != NULL);
  EnableWindow(m_BackHwnd, canGoBack);
  EnableWindow(m_ForwardHwnd, canGoForward);
#endif // SHOW_TOOLBAR_UI
}

void ClientHandler::CloseMainWindow() {
  ::PostMessage(m_MainHwnd, WM_CLOSE, 0, 0);
}
