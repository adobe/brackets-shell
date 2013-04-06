// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "config.h"
#include "client_handler.h"
#include <string>
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "resource.h"
#include "native_menu_model.h"

#include <ShellAPI.h>

#define CLOSING_PROP L"CLOSING"

extern CefRefPtr<ClientHandler> g_handler;

// WM_DROPFILES handler, defined in cefclient_win.cpp
extern LRESULT HandleDropFiles(HDROP hDrop, CefRefPtr<ClientHandler> handler, CefRefPtr<CefBrowser> browser);

// Additional globals
extern HACCEL hAccelTable;

bool ClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
                                  const CefPopupFeatures& popupFeatures,
                                  CefWindowInfo& windowInfo,
                                  const CefString& url,
                                  CefRefPtr<CefClient>& client,
                                  CefBrowserSettings& settings) {
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
          default:
            ExtensionString commandId = NativeMenuModel::getInstance(getMenuParent(browser)).getCommandId(wmId);
            if (commandId.size() > 0) {
              CefRefPtr<CommandCallback> callback = new EditCommandCallback(browser, commandId);
              g_handler->SendJSCommand(browser, commandId, callback);
            }
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

    case WM_DROPFILES:
      if (g_handler.get() && browser.get()) {
        return HandleDropFiles((HDROP)wParam, g_handler, browser);
      }
      break;

    case WM_INITMENUPOPUP:
        HMENU menu = (HMENU)wParam;
        int count = GetMenuItemCount(menu);
        void* menuParent = getMenuParent(browser);
        for (int i = 0; i < count; i++) {
            UINT id = GetMenuItemID(menu, i);

            bool enabled = NativeMenuModel::getInstance(menuParent).isMenuItemEnabled(id);
            UINT flagEnabled = enabled ? MF_ENABLED | MF_BYCOMMAND : MF_DISABLED | MF_BYCOMMAND;
            EnableMenuItem(menu, id,  flagEnabled);

            bool checked = NativeMenuModel::getInstance(menuParent).isMenuItemChecked(id);
            UINT flagChecked = checked ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
            CheckMenuItem(menu, id, flagChecked);
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
    DragAcceptFiles(hWnd, true);
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

    if (::TranslateAccelerator(frameHwnd, hAccelTable, os_event)) {
        return true;
    }

    return false;
}

bool ClientHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                                const CefKeyEvent& event,
                                CefEventHandle os_event) {
  return false;
}
