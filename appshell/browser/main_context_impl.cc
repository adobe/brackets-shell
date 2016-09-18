// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "appshell/browser/main_context_impl.h"

#include "include/cef_parser.h"
#include "appshell/common/client_switches.h"

#include "appshell/appshell_helpers.h"
#include "appshell/config.h"

namespace client {

namespace {

// The default URL to load in a browser window.
const char kDefaultUrl[] = "http://www.google.com";

}  // namespace

MainContextImpl::MainContextImpl(CefRefPtr<CefCommandLine> command_line,
                                 bool terminate_when_all_windows_closed)
    : command_line_(command_line),
      terminate_when_all_windows_closed_(terminate_when_all_windows_closed),
      initialized_(false),
      shutdown_(false),
      background_color_(CefColorSetARGB(255, 255, 255, 255)) {
  DCHECK(command_line_.get());

  // Set the main URL.
  if (command_line_->HasSwitch(switches::kUrl))
    main_url_ = command_line_->GetSwitchValue(switches::kUrl);
  if (main_url_.empty())
    main_url_ = kDefaultUrl;

  if (command_line_->HasSwitch(switches::kBackgroundColor)) {
    // Parse the background color value.
    CefParseCSSColor(command_line_->GetSwitchValue(switches::kBackgroundColor),
                     false, background_color_);
  }
}

MainContextImpl::~MainContextImpl() {
  // The context must either not have been initialized, or it must have also
  // been shut down.
  DCHECK(!initialized_ || shutdown_);
}

std::string MainContextImpl::GetConsoleLogPath() {
  return GetAppWorkingDirectory() + "console.log";
}

std::string MainContextImpl::GetMainURL() {
  return main_url_;
}

cef_color_t MainContextImpl::GetBackgroundColor() {
  return background_color_;
}

void MainContextImpl::PopulateSettings(CefSettings* settings) {
#if defined(OS_WIN)
  settings->multi_threaded_message_loop =
      command_line_->HasSwitch(switches::kMultiThreadedMessageLoop);
#endif

  CefString(&settings->cache_path) =
      command_line_->GetSwitchValue(switches::kCachePath);

  if (command_line_->HasSwitch(switches::kOffScreenRenderingEnabled))
    settings->windowless_rendering_enabled = true;

  settings->background_color = background_color_;

  CefString(&settings->log_file) =
      command_line_->GetSwitchValue(switches::kLogFile);

  {
    std::string str = command_line_->GetSwitchValue(switches::kLogSeverity);

    // Default to LOGSEVERITY_DISABLE
    settings->log_severity = LOGSEVERITY_DISABLE;

    if (!str.empty()) {
      if (str == client::switches::kLogSeverity_Verbose)
        settings->log_severity = LOGSEVERITY_VERBOSE;
      else if (str == client::switches::kLogSeverity_Info)
        settings->log_severity = LOGSEVERITY_INFO;
      else if (str == client::switches::kLogSeverity_Warning)
        settings->log_severity = LOGSEVERITY_WARNING;
      else if (str == client::switches::kLogSeverity_Error)
        settings->log_severity = LOGSEVERITY_ERROR;
      else if (str == client::switches::kLogSeverity_Disable)
        settings->log_severity = LOGSEVERITY_DISABLE;
    }
  }

  // Don't update the settings.locale with the locale that we detected from the OS.
  // Otherwise, CEF will use it to find the resources and when it fails in finding resources
  // for some locales that are not available in resources, it crashes.
  //CefString(&settings.locale) = app->GetCurrentLanguage( );

  CefString(&settings->javascript_flags) =
      command_line_->GetSwitchValue(switches::kJavascriptFlags);

  // Enable dev tools
  settings->remote_debugging_port = REMOTE_DEBUGGING_PORT;

  std::wstring versionStr = appshell::AppGetProductVersionString();

  if (!versionStr.empty()) {
    // Explicitly append the Chromium version to our own product version string
    // since assigning product version always replaces the Chromium version in
    // the User Agent string.
    versionStr.append(L" ");
    versionStr.append(appshell::AppGetChromiumVersionString());

    // Set product version, which gets added to the User Agent string
    CefString(&settings->product_version) = versionStr;
  }

#if defined(OS_LINUX)
  settings->no_sandbox = TRUE;
#elif defined(OS_MACOSX)
  settings->no_sandbox = YES;
#endif

  // Check cache_path setting
  if (CefString(&settings->cache_path).length() == 0) {
    CefString(&settings->cache_path) = appshell::AppGetCachePath();
  }
}

void MainContextImpl::PopulateBrowserSettings(CefBrowserSettings* settings) {
  if (command_line_->HasSwitch(switches::kOffScreenFrameRate)) {
    settings->windowless_frame_rate = atoi(command_line_->
        GetSwitchValue(switches::kOffScreenFrameRate).ToString().c_str());
  }

  settings->web_security = STATE_DISABLED;

  // Necessary to enable document.executeCommand("paste")
  settings->javascript_access_clipboard = STATE_ENABLED;
  settings->javascript_dom_paste = STATE_ENABLED;
}

RootWindowManager* MainContextImpl::GetRootWindowManager() {
  DCHECK(InValidState());
  return root_window_manager_.get();
}

bool MainContextImpl::Initialize(const CefMainArgs& args,
                                 const CefSettings& settings,
                                 CefRefPtr<CefApp> application,
                                 void* windows_sandbox_info) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!initialized_);
  DCHECK(!shutdown_);

  if (!CefInitialize(args, settings, application, windows_sandbox_info))
    return false;

  // Need to create the RootWindowManager after calling CefInitialize because
  // TempWindowX11 uses cef_get_xdisplay().
  root_window_manager_.reset(
      new RootWindowManager(terminate_when_all_windows_closed_));

  initialized_ = true;

  return true;
}

void MainContextImpl::Shutdown() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(initialized_);
  DCHECK(!shutdown_);

  root_window_manager_.reset();

  CefShutdown();

  shutdown_ = true;
}

}  // namespace client
