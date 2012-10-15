// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "client_handler.h"
#include <stdio.h>
#include <sstream>
#include <string>
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/wrapper/cef_stream_resource_handler.h"
#include "cefclient.h"
#include "resource_util.h"
#include "string_util.h"
#include "appshell/appshell_extensions.h"
#include "appshell/command_callbacks.h"

// Custom menu command Ids.
enum client_menu_ids {
  CLIENT_ID_SHOW_DEVTOOLS   = MENU_ID_USER_FIRST,
};

ClientHandler::BrowserWindowMap ClientHandler::browser_window_map_;

ClientHandler::ClientHandler()
  : m_MainHwnd(NULL),
    m_BrowserId(0),
    m_EditHwnd(NULL),
    m_BackHwnd(NULL),
    m_ForwardHwnd(NULL),
    m_StopHwnd(NULL),
    m_ReloadHwnd(NULL),
    m_bFormElementHasFocus(false),
    m_quitting(false) {
  callbackId = 0;
  CreateProcessMessageDelegates(process_message_delegates_);
  CreateRequestDelegates(request_delegates_);
}

ClientHandler::~ClientHandler() {
}

bool ClientHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  bool handled = false;
  
  // Check for callbacks first
  if (message->GetName() == "executeCommandCallback") {
    int32 commandId = message->GetArgumentList()->GetInt(0);
    bool result = message->GetArgumentList()->GetBool(1);
    
    CefRefPtr<CommandCallback> callback = command_callback_map_[commandId];
    callback->CommandComplete(result);
    command_callback_map_.erase(commandId);
    
    handled = true;
  }
  
  // Execute delegate callbacks.
  ProcessMessageDelegateSet::iterator it = process_message_delegates_.begin();
  for (; it != process_message_delegates_.end() && !handled; ++it) {
    handled = (*it)->OnProcessMessageReceived(this, browser, source_process,
                                              message);
  }
    
  return handled;
}

void ClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  AutoLock lock_scope(this);
  if (!m_Browser.get())   {
    // We need to keep the main child window, but not popup windows
    m_Browser = browser;
    m_BrowserId = browser->GetIdentifier();
  } else {
    // Call the platform-specific code to hook up popup windows
    PopupCreated(browser);
  }

  browser_window_map_[browser->GetHost()->GetWindowHandle()] = browser;
}

bool ClientHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier()) {
    // Since the main window contains the browser window, we need to close
    // the parent window instead of the browser window.
    CloseMainWindow();

    // Return true here so that we can skip closing the browser window
    // in this pass. (It will be destroyed due to the call to close
    // the parent above.)
    return true;
  }

  // A popup browser window is not contained in another window, so we can let
  // these windows close by themselves.
  return false;
}

void ClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  if (CanCloseBrowser(browser)) {
    if (m_BrowserId == browser->GetIdentifier()) {
      // Free the browser pointer so that the browser can be destroyed
      m_Browser = NULL;
	} else if (browser->IsPopup()) {
      // Remove the record for DevTools popup windows.
      std::set<std::string>::iterator it =
          m_OpenDevToolsURLs.find(browser->GetMainFrame()->GetURL());
      if (it != m_OpenDevToolsURLs.end())
        m_OpenDevToolsURLs.erase(it);
	}

    browser_window_map_.erase(browser->GetHost()->GetWindowHandle());
  }
  
  if (m_quitting) {
    DispatchCloseToNextBrowser();
  }
}

void ClientHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame) {
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
    // We've just started loading a page
    SetLoading(true);
  }
}

void ClientHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode) {
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
    // We've just finished loading a page
    SetLoading(false);
  }
}

void ClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
  REQUIRE_UI_THREAD();

  // Display a load error message.
  std::stringstream ss;
  ss << "<html><body><h2>Failed to load URL " << std::string(failedUrl) <<
        " with error " << std::string(errorText) << " (" << errorCode <<
        ").</h2></body></html>";
  frame->LoadString(ss.str(), failedUrl);
}

CefRefPtr<CefResourceHandler> ClientHandler::GetResourceHandler(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request) {
  CefRefPtr<CefResourceHandler> handler;

  // Execute delegate callbacks.
  RequestDelegateSet::iterator it = request_delegates_.begin();
  for (; it != request_delegates_.end() && !handler.get(); ++it)
    handler = (*it)->GetResourceHandler(this, browser, frame, request);

  return handler;
}

void ClientHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward) {
  REQUIRE_UI_THREAD();
  SetLoading(isLoading);
  SetNavState(canGoBack, canGoForward);
}

bool ClientHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                     const CefString& message,
                                     const CefString& source,
                                     int line) {
  // Don't write the message to a console.log file. Instead, we'll just
  // return false here so the message gets written to the console (output window
  // in xcode, or console window in dev tools)
  
/*
  REQUIRE_UI_THREAD();

  bool first_message;
  std::string logFile;

  {
    AutoLock lock_scope(this);

    first_message = m_LogFile.empty();
    if (first_message) {
      std::stringstream ss;
      ss << AppGetWorkingDirectory();
#if defined(OS_WIN)
      ss << "\\";
#else
      ss << "/";
#endif
      ss << "console.log";
      m_LogFile = ss.str();
    }
    logFile = m_LogFile;
  }

  FILE* file = fopen(logFile.c_str(), "a");
  if (file) {
    std::stringstream ss;
    ss << "Message: " << std::string(message) << "\r\nSource: " <<
        std::string(source) << "\r\nLine: " << line <<
        "\r\n-----------------------\r\n";
    fputs(ss.str().c_str(), file);
    fclose(file);

    if (first_message)
      SendNotification(NOTIFY_CONSOLE_MESSAGE);
  }
*/
  return false;
}

void ClientHandler::OnRequestGeolocationPermission(
      CefRefPtr<CefBrowser> browser,
      const CefString& requesting_url,
      int request_id,
      CefRefPtr<CefGeolocationCallback> callback) {
  // Allow geolocation access from all websites.
  callback->Continue(true);
}

void ClientHandler::OnBeforeContextMenu(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params,
    CefRefPtr<CefMenuModel> model) {
  if ((params->GetTypeFlags() & (CM_TYPEFLAG_PAGE | CM_TYPEFLAG_FRAME)) != 0) {
    // Add a separator if the menu already has items.
    if (model->GetCount() > 0)
      model->AddSeparator();

    // Add a "Show DevTools" item to all context menus.
    model->AddItem(CLIENT_ID_SHOW_DEVTOOLS, "&Show DevTools");

    CefString devtools_url = browser->GetHost()->GetDevToolsURL(true);
    if (devtools_url.empty() ||
        m_OpenDevToolsURLs.find(devtools_url) != m_OpenDevToolsURLs.end()) {
      // Disable the menu option if DevTools isn't enabled or if a window is
      // already open for the current URL.
      model->SetEnabled(CLIENT_ID_SHOW_DEVTOOLS, false);
    }
  }
}

bool ClientHandler::OnContextMenuCommand(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params,
    int command_id,
    EventFlags event_flags) {
  switch (command_id) {
    case CLIENT_ID_SHOW_DEVTOOLS:
      ShowDevTools(browser);
      return true;
    default:  // Allow default handling, if any.
      return false;
  }
}

void ClientHandler::SetMainHwnd(CefWindowHandle hwnd) {
  AutoLock lock_scope(this);
  m_MainHwnd = hwnd;
}

void ClientHandler::SetEditHwnd(CefWindowHandle hwnd) {
  AutoLock lock_scope(this);
  m_EditHwnd = hwnd;
}

void ClientHandler::SetButtonHwnds(CefWindowHandle backHwnd,
                                   CefWindowHandle forwardHwnd,
                                   CefWindowHandle reloadHwnd,
                                   CefWindowHandle stopHwnd) {
  AutoLock lock_scope(this);
  m_BackHwnd = backHwnd;
  m_ForwardHwnd = forwardHwnd;
  m_ReloadHwnd = reloadHwnd;
  m_StopHwnd = stopHwnd;
}

std::string ClientHandler::GetLogFile() {
  AutoLock lock_scope(this);
  return m_LogFile;
}

void ClientHandler::SetLastDownloadFile(const std::string& fileName) {
  AutoLock lock_scope(this);
  m_LastDownloadFile = fileName;
}

std::string ClientHandler::GetLastDownloadFile() {
  AutoLock lock_scope(this);
  return m_LastDownloadFile;
}

void ClientHandler::ShowDevTools(CefRefPtr<CefBrowser> browser) {
  std::string devtools_url = browser->GetHost()->GetDevToolsURL(true);
  if (!devtools_url.empty() &&
      m_OpenDevToolsURLs.find(devtools_url) == m_OpenDevToolsURLs.end()) {
    m_OpenDevToolsURLs.insert(devtools_url);
    browser->GetMainFrame()->ExecuteJavaScript(
        "window.open('" +  devtools_url + "');", "about:blank", 0);
  }
}

bool ClientHandler::SendJSCommand(CefRefPtr<CefBrowser> browser, const CefString& commandName, CefRefPtr<CommandCallback> callback)
{
  CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("executeCommand");
  message->GetArgumentList()->SetString(0, commandName);

  if (callback) {
    callbackId++;
    command_callback_map_[callbackId] = callback;
    message->GetArgumentList()->SetInt(1, callbackId);
  }

  return browser->SendProcessMessage(PID_RENDERER, message);
}

void ClientHandler::DispatchCloseToNextBrowser()
{
  // If the inner loop iterates thru all browsers and there's still at least one
  // left (i.e. the first browser that was skipped), then re-start loop
  while (browser_window_map_.size() > 0)
  {
    // Close the first (main) window last. On Windows, closing the main window exits the
    // application, so make sure all other windows get a crack at saving changes first.
    bool skipFirstOne = (browser_window_map_.size() > 1);

    BrowserWindowMap::const_iterator i;
    for (i = browser_window_map_.begin(); i != browser_window_map_.end(); i++)
    {
      if (skipFirstOne) {
        skipFirstOne = false;
        continue;
      }
      CefRefPtr<CefBrowser> browser = i->second;

      // Bring the window to the front before sending the command
      BringBrowserWindowToFront(browser);
    
      // This call initiates a quit sequence. We will continue to close browser windows
      // unless AbortQuit() is called.
      m_quitting = true;
      CefRefPtr<CommandCallback> callback = new CloseWindowCommandCallback(browser);

      if (SendJSCommand(browser, FILE_CLOSE_WINDOW, callback)) {
        // JS Command successfully sent, so we're done
        return;
      }

      // Sending JS command to browser failed, so remove it from queue, continue to next browser
      browser_window_map_.erase(i->first);
    }
  }
}

void ClientHandler::AbortQuit()
{
	m_quitting = false;

	// Notify all browsers that quit was aborted
	BrowserWindowMap::const_iterator i, last = browser_window_map_.end();
	for (i = browser_window_map_.begin(); i != last; i++)
	{
		CefRefPtr<CefBrowser> browser = i->second;
		SendJSCommand(browser, APP_ABORT_QUIT);
	}
}

// static
void ClientHandler::CreateProcessMessageDelegates(
      ProcessMessageDelegateSet& delegates) {
	appshell_extensions::CreateProcessMessageDelegates(delegates);
}

// static
void ClientHandler::CreateRequestDelegates(RequestDelegateSet& delegates) {
}
