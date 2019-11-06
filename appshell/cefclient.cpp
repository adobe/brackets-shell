// Copyright (c) 2010 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient.h"
#include <stdio.h>
#include <cstdlib>
#include <sstream>
#include <string>
#include <limits>
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
int g_remote_debugging_port = 0;

#ifdef OS_WIN
bool g_force_enable_acc = false;
#undef max
#endif

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

// CefCommandLine::HasSwitch is unable to report the presense of switches,
// in the command line properly. This is a generic function that could be
// used to check for any particular switch, passed as a command line argument.
bool HasSwitch(CefRefPtr<CefCommandLine> command_line , CefString& switch_name)
{
  if (command_line) {
    ExtensionString cmdLine = command_line->GetCommandLineString();
    size_t idx = cmdLine.find(switch_name);
    return idx > 0 && idx < cmdLine.length();
  } else {
    return false;
  }
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
  CefString debugger_port = command_line->GetSwitchValue("remote-debugging-port");
  if (!debugger_port.empty()) {
    int port = atoi(debugger_port.ToString().c_str());
    static const int max_port_num = static_cast<int>(std::numeric_limits<uint16_t>::max());
    if (port > 1024 && port < max_port_num) {
      g_remote_debugging_port = port;
      settings.remote_debugging_port = port;
    }
    else {
      LOG(ERROR) << "Could not enable remote debugging on port: "<< port
                 << "; port number must be greater than 1024 and less than " << max_port_num;
    }
  }
  
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

#ifdef OS_WIN
  // We disable renderer accessibility by default as it is known to cause performance
  // issues. But if any one wants to enable it back, then we need to honor the flag.

  CefString force_acc_switch_name("--force-renderer-accessibility");
  CefString enable_acc_switch_name("--enable-renderer-accessibility");

  if (HasSwitch(command_line, force_acc_switch_name) || HasSwitch(command_line, enable_acc_switch_name))
    g_force_enable_acc = true;
#endif

}
