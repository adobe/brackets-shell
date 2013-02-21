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

  CefString str;

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
      else if (str == cefclient::kLogSeverity_ErrorReport)
        settings.log_severity = LOGSEVERITY_ERROR_REPORT;
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

  // Retrieve command-line proxy configuration, if any.
  bool has_proxy = false;
  cef_proxy_type_t proxy_type = CEF_PROXY_TYPE_DIRECT;
  CefString proxy_config;

  if (g_command_line->HasSwitch(cefclient::kProxyType)) {
    std::string str = g_command_line->GetSwitchValue(cefclient::kProxyType);
    if (str == cefclient::kProxyType_Direct) {
      has_proxy = true;
      proxy_type = CEF_PROXY_TYPE_DIRECT;
    } else if (str == cefclient::kProxyType_Named ||
               str == cefclient::kProxyType_Pac) {
      proxy_config = g_command_line->GetSwitchValue(cefclient::kProxyConfig);
      if (!proxy_config.empty()) {
        has_proxy = true;
        proxy_type = (str == cefclient::kProxyType_Named?
                      CEF_PROXY_TYPE_NAMED:CEF_PROXY_TYPE_PAC_STRING);
      }
    }
  }

  if (has_proxy) {
    // Provide a ClientApp instance to handle proxy resolution.
    app->SetProxyConfig(proxy_type, proxy_config);
  }
    
  // Enable dev tools
  settings.remote_debugging_port = REMOTE_DEBUGGING_PORT;
  
  // Set product version, which gets added to the User Agent string
  CefString(&settings.product_version) = AppGetProductVersionString();

}

// Returns the application browser settings based on command line arguments.
void AppGetBrowserSettings(CefBrowserSettings& settings) {
  ASSERT(g_command_line.get());
  if (!g_command_line.get())
    return;

  settings.remote_fonts_disabled =
      g_command_line->HasSwitch(cefclient::kRemoteFontsDisabled);

  CefString(&settings.default_encoding) =
      g_command_line->GetSwitchValue(cefclient::kDefaultEncoding);

  settings.encoding_detector_enabled =
      g_command_line->HasSwitch(cefclient::kEncodingDetectorEnabled);
  settings.javascript_disabled =
      g_command_line->HasSwitch(cefclient::kJavascriptDisabled);
  settings.javascript_open_windows_disallowed =
      g_command_line->HasSwitch(cefclient::kJavascriptOpenWindowsDisallowed);
  settings.javascript_close_windows_disallowed =
      g_command_line->HasSwitch(cefclient::kJavascriptCloseWindowsDisallowed);
  settings.javascript_access_clipboard_disallowed =
      g_command_line->HasSwitch(
          cefclient::kJavascriptAccessClipboardDisallowed);
  settings.dom_paste_disabled =
      g_command_line->HasSwitch(cefclient::kDomPasteDisabled);
  settings.caret_browsing_enabled =
      g_command_line->HasSwitch(cefclient::kCaretBrowsingDisabled);
  settings.java_disabled =
      g_command_line->HasSwitch(cefclient::kJavaDisabled);
  settings.plugins_disabled =
      g_command_line->HasSwitch(cefclient::kPluginsDisabled);
  settings.universal_access_from_file_urls_allowed =
      g_command_line->HasSwitch(cefclient::kUniversalAccessFromFileUrlsAllowed);
  settings.file_access_from_file_urls_allowed =
      g_command_line->HasSwitch(cefclient::kFileAccessFromFileUrlsAllowed);
  settings.web_security_disabled =
      g_command_line->HasSwitch(cefclient::kWebSecurityDisabled);
  settings.xss_auditor_enabled =
      g_command_line->HasSwitch(cefclient::kXssAuditorEnabled);
  settings.image_load_disabled =
      g_command_line->HasSwitch(cefclient::kImageLoadingDisabled);
  settings.shrink_standalone_images_to_fit =
      g_command_line->HasSwitch(cefclient::kShrinkStandaloneImagesToFit);
  settings.site_specific_quirks_disabled =
      g_command_line->HasSwitch(cefclient::kSiteSpecificQuirksDisabled);
  settings.text_area_resize_disabled =
      g_command_line->HasSwitch(cefclient::kTextAreaResizeDisabled);
  settings.page_cache_disabled =
      g_command_line->HasSwitch(cefclient::kPageCacheDisabled);
  settings.tab_to_links_disabled =
      g_command_line->HasSwitch(cefclient::kTabToLinksDisabled);
  settings.hyperlink_auditing_disabled =
      g_command_line->HasSwitch(cefclient::kHyperlinkAuditingDisabled);
  settings.user_style_sheet_enabled =
      g_command_line->HasSwitch(cefclient::kUserStyleSheetEnabled);

  CefString(&settings.user_style_sheet_location) =
      g_command_line->GetSwitchValue(cefclient::kUserStyleSheetLocation);

  settings.author_and_user_styles_disabled =
      g_command_line->HasSwitch(cefclient::kAuthorAndUserStylesDisabled);
  settings.local_storage_disabled =
      g_command_line->HasSwitch(cefclient::kLocalStorageDisabled);
  settings.databases_disabled =
      g_command_line->HasSwitch(cefclient::kDatabasesDisabled);
  settings.application_cache_disabled =
      g_command_line->HasSwitch(cefclient::kApplicationCacheDisabled);
  settings.webgl_disabled =
      g_command_line->HasSwitch(cefclient::kWebglDisabled);
  settings.accelerated_compositing_disabled =
      g_command_line->HasSwitch(cefclient::kAcceleratedCompositingDisabled);
  settings.accelerated_layers_disabled =
      g_command_line->HasSwitch(cefclient::kAcceleratedLayersDisabled);
  settings.accelerated_video_disabled =
      g_command_line->HasSwitch(cefclient::kAcceleratedVideoDisabled);
  settings.accelerated_2d_canvas_disabled =
      g_command_line->HasSwitch(cefclient::kAcceledated2dCanvasDisabled);
  settings.accelerated_painting_enabled =
      g_command_line->HasSwitch(cefclient::kAcceleratedPaintingEnabled);
  settings.accelerated_filters_enabled =
      g_command_line->HasSwitch(cefclient::kAcceleratedFiltersEnabled);
  settings.accelerated_plugins_disabled =
      g_command_line->HasSwitch(cefclient::kAcceleratedPluginsDisabled);
  settings.developer_tools_disabled =
      g_command_line->HasSwitch(cefclient::kDeveloperToolsDisabled);
  settings.fullscreen_enabled =
      g_command_line->HasSwitch(cefclient::kFullscreenEnabled);
}