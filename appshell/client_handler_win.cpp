// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
#define OS_WIN
#include "config.h"
#include "client_handler.h"
#include <string>
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "resource.h"
#include "native_menu_model.h"

#include <ShellAPI.h>

#include "cef_popup_window.h"

#include "cef_main_window.h"
extern cef_main_window* gMainWnd;


// Additional globals
extern HACCEL hAccelTable;

void ClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  REQUIRE_UI_THREAD();
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
}

void ClientHandler::SetNavState(bool canGoBack, bool canGoForward) {
}

void ClientHandler::CloseMainWindow() {
  ::PostMessage(m_MainHwnd, WM_CLOSE, 0, 0);
}

void ClientHandler::PopupCreated(CefRefPtr<CefBrowser> browser)
{
    HWND hWnd = browser->GetHost()->GetWindowHandle();
    cef_popup_window* pw = new cef_popup_window(browser);
    pw->SubclassWindow(hWnd);
}

CefRefPtr<CefBrowser> ClientHandler::GetBrowserForNativeWindow(void* window) {
  CefRefPtr<CefBrowser> browser = NULL;
  if (window) {
    //go through all the browsers looking for a browser within this window
    BrowserWindowMap::const_iterator i = browser_window_map_.find((HWND)window);
    if (i != browser_window_map_.end()) {
      browser = i->second;
    }
  }
  return browser;
}

bool ClientHandler::CanCloseBrowser(CefRefPtr<CefBrowser> browser) {

  // On windows, the main browser needs to be destroyed last.
  // So, don't allow main browser to be closed until there's
  // only 1 browser remaining in the map.
  return (browser_window_map_.size() == 1) || 
         (browser && browser->GetIdentifier() != m_BrowserId);
}

bool ClientHandler::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                                    const CefKeyEvent& event,
                                    CefEventHandle os_event,
                                    bool* is_keyboard_shortcut) {
    HWND frameHwnd = (HWND)getMenuParent(browser);

    // Don't call ::TranslateAccelerator if we don't have a menu for the current window.
    if (!GetMenu(frameHwnd)) {
        return false;
    }
    
    if (os_event && ::TranslateAccelerator(frameHwnd, hAccelTable, os_event)) {
        return true;
    }

    return false;
}

bool ClientHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                                const CefKeyEvent& event,
                                CefEventHandle os_event) {
  return false;
}


 void ClientHandler::ComputePopupPlacement(CefWindowInfo& windowInfo)
 {
      // both must be set to work
      if (windowInfo.width == CW_USEDEFAULT ||
          windowInfo.height == CW_USEDEFAULT) {
            // force both vals to be CW_USEDEFAULT in this
            //  case because Windows doesn't correctly handle 
            //  only one value being is supplied on input
            windowInfo.width = CW_USEDEFAULT;
            windowInfo.height = CW_USEDEFAULT;
         return;
     }
	 
     RECT rectMainWnd;
     gMainWnd->GetWindowRect(&rectMainWnd);

     int mW = ::RectWidth(rectMainWnd);
     int mH = ::RectHeight(rectMainWnd);
     int cW = windowInfo.width;
     int cH = windowInfo.height;

     windowInfo.x = (rectMainWnd.left + (mW /2)) - (cW / 2);
     windowInfo.y = (rectMainWnd.top + (mH /2)) - (cH / 2);

     // don't go offscreen
     if (windowInfo.x < 0) windowInfo.x = 0;
     if (windowInfo.y < 0) windowInfo.y = 0;

     HMONITOR hm = ::MonitorFromWindow(gMainWnd->GetSafeWnd(), MONITOR_DEFAULTTONEAREST); 
     MONITORINFO mi = {0};
     mi.cbSize = sizeof (mi);

     GetMonitorInfo(hm, &mi);

     if (windowInfo.width > ::RectWidth(mi.rcWork)) {
         windowInfo.x = mi.rcWork.left;
         windowInfo.width = mi.rcWork.right - windowInfo.x;
     }

     if (windowInfo.height > ::RectHeight(mi.rcWork)) {
         windowInfo.y = mi.rcWork.top;
         windowInfo.height = mi.rcWork.bottom - windowInfo.y;
     }
 }
 
