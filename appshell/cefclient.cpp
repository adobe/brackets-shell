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
#include "include/cef_runnable.h"
#include "include/cef_web_plugin.h"
#include "client_handler.h"
#include "client_switches.h"
#include "string_util.h"
#include "util.h"
#include "config.h"

CefRefPtr<ClientHandler> g_handler;
CefRefPtr<CefCommandLine> g_command_line;

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

void AppInitCommandLine(int argc, const char* const* argv) {
  g_command_line = CefCommandLine::CreateCommandLine();
#if defined(OS_WIN)
  g_command_line->InitFromString(::GetCommandLineW());
#else
  g_command_line->InitFromArgv(argc, argv);
#endif
}

// Returns the application command line object.
CefRefPtr<CefCommandLine> AppGetCommandLine() {
  return g_command_line;
}

// Returns the application settings based on command line arguments.
void AppGetSettings(CefSettings& settings, CefRefPtr<ClientApp> app) {
  ASSERT(app.get());
  ASSERT(g_command_line.get());
  if (!g_command_line.get())
    return;

#if defined(OS_WIN)
  settings.multi_threaded_message_loop =
      g_command_line->HasSwitch(cefclient::kMultiThreadedMessageLoop);
#endif

  CefString(&settings.cache_path) =
      g_command_line->GetSwitchValue(cefclient::kCachePath);
  CefString(&settings.log_file) =
      g_command_line->GetSwitchValue(cefclient::kLogFile);

  {
    std::string str = g_command_line->GetSwitchValue(cefclient::kLogSeverity);

    // Default to LOGSEVERITY_DISABLE
    settings.log_severity = LOGSEVERITY_DISABLE;

    if (!str.empty()) {
      if (str == cefclient::kLogSeverity_Verbose)
        settings.log_severity = LOGSEVERITY_VERBOSE;
      else if (str == cefclient::kLogSeverity_Info)
        settings.log_severity = LOGSEVERITY_INFO;
      else if (str == cefclient::kLogSeverity_Warning)
        settings.log_severity = LOGSEVERITY_WARNING;
      else if (str == cefclient::kLogSeverity_Error)
        settings.log_severity = LOGSEVERITY_ERROR;
      else if (str == cefclient::kLogSeverity_Disable)
        settings.log_severity = LOGSEVERITY_DISABLE;
    }
  }

  // Don't update the settings.locale with the locale that we detected from the OS.
  // Otherwise, CEF will use it to find the resources and when it fails in finding resources
  // for some locales that are not available in resources, it crashes.
  //CefString(&settings.locale) = app->GetCurrentLanguage( );

  CefString(&settings.javascript_flags) =
      g_command_line->GetSwitchValue(cefclient::kJavascriptFlags);
    
  // Enable dev tools
  settings.remote_debugging_port = REMOTE_DEBUGGING_PORT;
  
  std::wstring versionStr = AppGetProductVersionString();
    
  if (!versionStr.empty()) {
    // Explicitly append the Chromium version to our own product version string
    // since assigning product version always replaces the Chromium version in
    // the User Agent string.
    versionStr.append(L" ");
    versionStr.append(AppGetChromiumVersionString());
      
    // Set product version, which gets added to the User Agent string
    CefString(&settings.product_version) = versionStr;
  }
}
