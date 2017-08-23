// Copyright (c) 2010 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient.h"
#include <stdio.h>
#include <cstdlib>
#include <sstream>
#include <string>
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/cef_frame.h"
#include "include/cef_web_plugin.h"
#include "include/base/cef_logging.h"
#include "client_handler.h"
#include "appshell/common/client_switches.h"
#include "appshell/appshell_helpers.h"
#include "config.h"

CefRefPtr<ClientHandler> g_handler;

CefRefPtr<CefBrowser> AppGetBrowser() {
  if (!g_handler.get())
    return NULL;
  return g_handler->GetBrowser();
}

CefWindowHandle AppGetMainHwnd() {
  if (!g_handler.get())
    return NULL;
  return g_handler->GetMainHwnd();
}

// Returns the application settings based on command line arguments.
void AppGetSettings(CefSettings& settings, CefRefPtr<CefCommandLine> command_line) {
  DCHECK(command_line.get());
  if (!command_line.get())
    return;

#if defined(OS_WIN)
  settings.multi_threaded_message_loop =
      command_line->HasSwitch(client::switches::kMultiThreadedMessageLoop);
#endif

  CefString(&settings.cache_path) =
      command_line->GetSwitchValue(client::switches::kCachePath);
  CefString(&settings.log_file) =
      command_line->GetSwitchValue(client::switches::kLogFile);

  {
    std::string str = command_line->GetSwitchValue(client::switches::kLogSeverity);

    // Default to LOGSEVERITY_DISABLE
    settings.log_severity = LOGSEVERITY_DISABLE;

    if (!str.empty()) {
      if (str == client::switches::kLogSeverity_Verbose)
        settings.log_severity = LOGSEVERITY_VERBOSE;
      else if (str == client::switches::kLogSeverity_Info)
        settings.log_severity = LOGSEVERITY_INFO;
      else if (str == client::switches::kLogSeverity_Warning)
        settings.log_severity = LOGSEVERITY_WARNING;
      else if (str == client::switches::kLogSeverity_Error)
        settings.log_severity = LOGSEVERITY_ERROR;
      else if (str == client::switches::kLogSeverity_Disable)
        settings.log_severity = LOGSEVERITY_DISABLE;
    }
  }

  // Don't update the settings.locale with the locale that we detected from the OS.
  // Otherwise, CEF will use it to find the resources and when it fails in finding resources
  // for some locales that are not available in resources, it crashes.
  //CefString(&settings.locale) = appshell::GetCurrentLanguage( );

  CefString(&settings.javascript_flags) =
      command_line->GetSwitchValue(client::switches::kJavascriptFlags);
    
  // Enable dev tools
  settings.remote_debugging_port = REMOTE_DEBUGGING_PORT;
  
  std::wstring versionStr = appshell::AppGetProductVersionString();
    
  if (!versionStr.empty()) {
    // Explicitly append the Chromium version to our own product version string
    // since assigning product version always replaces the Chromium version in
    // the User Agent string.
    versionStr.append(L" ");
    versionStr.append(appshell::AppGetChromiumVersionString());
      
    // Set product version, which gets added to the User Agent string
    CefString(&settings.product_version) = versionStr;
  }
}
