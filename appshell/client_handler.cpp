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

#ifndef OS_LINUX

// CefWIndowInfo.height/.width aren't impelemented on Linux for some reason
//  we'll want to revisit this when we integrate the next version of CEF

static void SetValue(const std::string& name, const std::string& value, CefWindowInfo& windowInfo) {
    if (name == "height") {
        windowInfo.height = ::atoi(value.c_str());
    } else if (name == "width") {
        windowInfo.width = ::atoi(value.c_str());
    }
 }

static void ParseParams(const std::string& params, CefWindowInfo& windowInfo) {
    std::string name;
    std::string value;
    bool foundAssignmentToken = false;

    for (unsigned i = 0; i < params.length(); i++) {
        if (params[i] == '&') {
            SetValue(name, value, windowInfo);
            name.clear();
            value.clear();
            foundAssignmentToken = false;
        } else if (params[i] == '=') {
            foundAssignmentToken = true;
        } else if (!foundAssignmentToken) {
            name += params[i];
        } else {
            value+= params[i];
        }
    }

    // set the last parsed value that didn't end with an &
    SetValue(name, value, windowInfo);
 }

#endif

bool ClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             const CefString& target_url,
                             const CefString& target_frame_name,
                             const CefPopupFeatures& popupFeatures,
                             CefWindowInfo& windowInfo,
                             CefRefPtr<CefClient>& client,
                             CefBrowserSettings& settings,
                             bool* no_javascript_access) {
#ifndef OS_LINUX
    std::string address = target_url.ToString();
    std::string url;
    std::string params;
    bool foundParamToken = false;

    // make the input lower-case (easier string matching)
    std::transform(address.begin(), address.end(), address.begin(), ::tolower);

    for (unsigned i = 0; i < address.length(); i++) {
        if (!foundParamToken) {
            if (address[i] == L'?') {
                foundParamToken = true;
            } else {
                url += address[i];
            }
        } else {
            params += address[i];
        }
    }

    if (url == "about:blank") {
        ParseParams(params, windowInfo);
        ComputePopupPlacement(windowInfo);
    }
#endif
    return false;
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

void ClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  if (CanCloseBrowser(browser)) {
    if (m_BrowserId == browser->GetIdentifier()) {
      // Free the browser pointer so that the browser can be destroyed
      m_Browser = NULL;
	}

    browser_window_map_.erase(browser->GetHost()->GetWindowHandle());
  }
  
  if (m_quitting) {
    // Changed the logic to call CefQuitMesaageLoop()
    // for windows as it was crashing with 2171 CEF.
    if (HasWindows())
      DispatchCloseToNextBrowser();
    else
      CefQuitMessageLoop();
  }
}

std::vector<CefString> gDroppedFiles;

bool ClientHandler::OnDragEnter(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                DragOperationsMask mask) {
    REQUIRE_UI_THREAD();
    
    if (dragData->IsFile()) {
        gDroppedFiles.clear();
        // Store the dragged files in a vector for later use
        dragData->GetFileNames(gDroppedFiles);
    }
    return false;
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
  
  ss << "<html>" <<
        "<head>" <<
        "  <style type='text/css'>" <<
        "    body { background: #3c3f41; width:100%; height:100%; margin: 0; padding: 0; }" <<
        "    .logo { background-image: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAMdJREFUeNpi0Vz9SICBgWE9EDswkAYOAHEgE5maGaB61rPANGdr8QMxH1E6p177BMQfwYawwARBmv38/FAUbtq0CUxjE4cawMDEQCFgwSYIs5mQS8AG/P//nyybYfpYGP6T6fb/1HLB/39kGgDVx/L37z+yDIDpY/n35y+GJCy08YU+TB/L719/yXIBTB/L75+/wIyeky8x4h9XugCpheljFOw4vf8/eZmJgRGYI1l+/vgdCPTPemC0kGQIIyPjASYW5kCAAAMA5Oph7ZyIYMQAAAAASUVORK5CYII='); }"
        "    .debug { cursor: hand; position: absolute; bottom: 16px; right: 16px; width: 16px; height: 16px; font-family: sans-serif; font-size: .75em; color: #999; }" <<
        "  </style>" <<
        "  <script type='text/javascript'>" <<
        "    var url = '" << std::string(failedUrl) << "';" <<
        "    var errorText = '" << std::string(errorText) << "';" <<
        "    var errorCode = '" << errorCode << "';" <<
        "    var msg = 'Failed to load URL ' + url + ' with error: ' + errorText + ' (' + errorCode + ')';" <<
        "    console.error(msg);" <<
        "  </script>" <<
        "</head>" <<
        "<body><a class='debug logo' onclick='brackets.app.showDeveloperTools()' title='Click to view loading error in Developer Tools'>&nbsp;</a></body></html>";
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

bool ClientHandler::OnRequestGeolocationPermission(
      CefRefPtr<CefBrowser> browser,
      const CefString& requesting_url,
      int request_id,
      CefRefPtr<CefGeolocationCallback> callback) {
  // Allow geolocation access from all websites.
  // TODO: What does ref app do?
  callback->Continue(true);
  return true;
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
    CefWindowInfo wi;
    CefBrowserSettings settings;

#if defined(OS_WIN)
    wi.SetAsPopup(NULL, "DevTools");
#endif
    browser->GetHost()->ShowDevTools(wi, browser->GetHost()->GetClient(), settings, CefPoint());

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

void ClientHandler::SendOpenFileCommand(CefRefPtr<CefBrowser> browser, const CefString &fileArray) {
  std::string fileArrayStr(fileArray);
  // FIXME: Use SendJSCommand once it supports parameters
  std::string cmd = "require('command/CommandManager').execute('file.openDroppedFiles'," + fileArrayStr + ")";

  // if files are droppend and the Open Dialog is visible, then browser is NULL
  // This fixes https://github.com/adobe/brackets/issues/7752
  if (browser) {
    browser->GetMainFrame()->ExecuteJavaScript(CefString(cmd.c_str()),
                                browser->GetMainFrame()->GetURL(), 0);
  }
}

void ClientHandler::DispatchCloseToNextBrowser()
{
  // If the inner loop iterates thru all browsers and there's still at least one
  // left (i.e. the first browser that was skipped), then re-start loop
  while (browser_window_map_.size() > 0)
  {
    // Close the main window last. On Windows, closing the main window exits the
    // application, so make sure all other windows get a crack at saving changes first.
    bool skipMainWindow = (browser_window_map_.size() > 1);

    BrowserWindowMap::const_iterator i;
    for (i = browser_window_map_.begin(); i != browser_window_map_.end(); i++)
    {
      CefRefPtr<CefBrowser> browser = i->second;
      if (skipMainWindow && browser && browser->GetIdentifier() == m_BrowserId) {
        continue;
      }

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
