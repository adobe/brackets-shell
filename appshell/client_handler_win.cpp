// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "config.h"
#include "client_handler.h"
#include <string>
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "resource.h"

#define CLOSING_PROP L"CLOSING"

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

bool ClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
                                  const CefPopupFeatures& popupFeatures,
                                  CefWindowInfo& windowInfo,
                                  const CefString& url,
                                  CefRefPtr<CefClient>& client,
                                  CefBrowserSettings& settings) {
  REQUIRE_UI_THREAD();

  std::string urlStr = url;
  
  //ensure all non-dev tools windows get a menu bar
  if (windowInfo.menu == NULL && urlStr.find("chrome-devtools:") == std::string::npos) {
    windowInfo.menu = ::LoadMenu( GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_CEFCLIENT_POPUP) );
  }

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

static WNDPROC g_popupWndOldProc = NULL;
//BRACKETS: added so our popup windows can have a menu bar too
//
//  FUNCTION: PopupWndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Handle commands from the menus.
LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  //For now, we are only interest in WM_COMMAND's that we know are in our menus
  //and WM_CLOSE so we can delegate that back to the browser
  CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow(hWnd);
  switch (message)
  {
      
    case WM_COMMAND:
      {
        int wmId    = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
          case IDM_ABOUT:
            if (browser.get()) {
              g_handler->SendJSCommand(browser, HELP_ABOUT);
            }
            return 0;
          case IDM_CLOSE:
            if (g_handler.get() && browser.get()) {
              HWND browserHwnd = browser->GetHost()->GetWindowHandle();
              HANDLE closing = GetProp(browserHwnd, CLOSING_PROP);
              if (closing) {
                RemoveProp(browserHwnd, CLOSING_PROP);
                break;
              }

 		      CefRefPtr<CommandCallback> callback = new CloseWindowCommandCallback(browser);
 		      g_handler->SendJSCommand(browser, FILE_CLOSE_WINDOW, callback);
			}
			return 0;
          }
	  }
      break;

    case WM_CLOSE:
      if (g_handler.get() && browser.get()) {
        HWND browserHwnd = browser->GetHost()->GetWindowHandle();
        HANDLE closing = GetProp(browserHwnd, CLOSING_PROP);
        if (closing) {
            RemoveProp(browserHwnd, CLOSING_PROP);
            break;
        }

        CefRefPtr<CommandCallback> callback = new CloseWindowCommandCallback(browser);
        g_handler->SendJSCommand(browser, FILE_CLOSE_WINDOW, callback);
 		return 0;
      }
      break;
  }

  if (g_popupWndOldProc) 
    return (LRESULT)::CallWindowProc(g_popupWndOldProc, hWnd, message, wParam, lParam);
  return ::DefWindowProc(hWnd, message, wParam, lParam);
}

void AttachWindProcToPopup(HWND wnd)
{
  if (!wnd) {
    return;
  }

  if (!::GetMenu(wnd)) {
    return; //no menu, no need for the proc
  }

  WNDPROC curProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(wnd, GWLP_WNDPROC));

  //if this is the first time, assume the above checks are all we need, otherwise
  //it had better be the same proc we pulled before
  if (!g_popupWndOldProc) {
    g_popupWndOldProc = curProc;
  } else if (g_popupWndOldProc != curProc) {
    return;
  }

  SetWindowLongPtr(wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(PopupWndProc));
}

void LoadWindowsIcons(HWND wnd)
{
    if (!wnd) {
        return;
    }
    //We need to load the icons after the pop up is created so they have the
    //brackets icon instead of the generic window icon
    HINSTANCE inst = ::GetModuleHandle(NULL);
    HICON bigIcon = ::LoadIcon(inst, MAKEINTRESOURCE(IDI_CEFCLIENT));
    HICON smIcon = ::LoadIcon(inst, MAKEINTRESOURCE(IDI_SMALL));
    if(bigIcon) {
        ::SendMessage(wnd, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);
    }
    if(smIcon) {
        ::SendMessage(wnd, WM_SETICON, ICON_SMALL, (LPARAM)smIcon);
    }
}

void ClientHandler::PopupCreated(CefRefPtr<CefBrowser> browser)
{
    HWND hWnd = browser->GetHost()->GetWindowHandle();
    AttachWindProcToPopup(hWnd);
    LoadWindowsIcons(hWnd);
    browser->GetHost()->SetFocus(true);
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

  // On windows, the main browser is the first in the map. It needs to be
  // destroyed last. So, don't allow main browser to be closed until there's
  // only 1 browser remaining in the map.
  if (browser_window_map_.size() > 1) {
    CefWindowHandle hWndBrowser = browser->GetHost()->GetWindowHandle();
    CefWindowHandle hWndMain    = browser_window_map_.begin()->first;

    return (hWndBrowser != hWndMain);
  }

  return true;
}
