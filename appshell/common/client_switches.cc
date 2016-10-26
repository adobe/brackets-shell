// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "appshell/common/client_switches.h"

namespace client {
namespace switches {

const char kStartupPath[] = "startup-path";

// CEF and Chromium support a wide range of command-line switches. This file
// only contains command-line switches specific to the cefclient application.
// View CEF/Chromium documentation or search for *_switches.cc files in the
// Chromium source code to identify other existing command-line switches.
// Below is a partial listing of relevant *_switches.cc files:
//   base/base_switches.cc
//   cef/libcef/common/cef_switches.cc
//   chrome/common/chrome_switches.cc (not all apply)
//   content/public/common/content_switches.cc

const char kMultiThreadedMessageLoop[] = "multi-threaded-message-loop";
const char kCachePath[] = "cache-path";
const char kUrl[] = "url";
const char kOffScreenRenderingEnabled[] = "off-screen-rendering-enabled";
const char kOffScreenFrameRate[] = "off-screen-frame-rate";
const char kTransparentPaintingEnabled[] = "transparent-painting-enabled";
const char kShowUpdateRect[] = "show-update-rect";
const char kMouseCursorChangeDisabled[] = "mouse-cursor-change-disabled";
const char kRequestContextPerBrowser[] = "request-context-per-browser";
const char kRequestContextSharedCache[] = "request-context-shared-cache";
const char kBackgroundColor[] = "background-color";
const char kEnableGPU[] = "enable-gpu";
const char kFilterURL[] = "filter-url";

// CefSettings attributes.
const char kLogFile[] = "log-file";
const char kLogSeverity[] = "log-severity";
const char kLogSeverity_Verbose[] = "verbose";
const char kLogSeverity_Info[] = "info";
const char kLogSeverity_Warning[] = "warning";
const char kLogSeverity_Error[] = "error";
const char kLogSeverity_ErrorReport[] = "error-report";
const char kLogSeverity_Disable[] = "disable";
const char kGraphicsImpl[] = "graphics-implementation";
const char kGraphicsImpl_Angle[] = "angle";
const char kGraphicsImpl_AngleCmdBuffer[] = "angle-command-buffer";
const char kGraphicsImpl_Desktop[] = "desktop";
const char kGraphicsImpl_DesktopCmdBuffer[] = "desktop-command-buffer";
const char kLocalStorageQuota[] = "local-storage-quota";
const char kSessionStorageQuota[] = "session-storage-quota";
const char kJavascriptFlags[] = "javascript-flags";

// CefBrowserSettings attributes.
const char kDragDropDisabled[] = "drag-drop-disabled";
const char kLoadDropsDisabled[] = "load-drops-disabled";
const char kHistoryDisabled[] = "history-disabled";
const char kRemoteFontsDisabled[] = "remote-fonts-disabled";
const char kDefaultEncoding[] = "default-encoding";
const char kEncodingDetectorEnabled[] = "encoding-detector-enabled";
const char kJavascriptDisabled[] = "javascript-disabled";
const char kJavascriptOpenWindowsDisallowed[] =
    "javascript-open-windows-disallowed";
const char kJavascriptCloseWindowsDisallowed[] =
    "javascript-close-windows-disallowed";
const char kJavascriptAccessClipboardDisallowed[] =
    "javascript-access-clipboard-disallowed";
const char kDomPasteDisabled[] = "dom-paste-disabled";
const char kCaretBrowsingDisabled[] = "caret-browsing-enabled";
const char kJavaDisabled[] = "java-disabled";
const char kPluginsDisabled[] = "plugins-disabled";
const char kUniversalAccessFromFileUrlsAllowed[] =
    "universal-access-from-file-urls-allowed";
const char kFileAccessFromFileUrlsAllowed[] =
    "file-access-from-file-urls-allowed";
const char kWebSecurityDisabled[] = "web-security-disabled";
const char kXssAuditorEnabled[] = "xss-auditor-enabled";
const char kImageLoadingDisabled[] = "image-load-disabled";
const char kShrinkStandaloneImagesToFit[] = "shrink-standalone-images-to-fit";
const char kSiteSpecificQuirksDisabled[] = "site-specific-quirks-disabled";
const char kTextAreaResizeDisabled[] = "text-area-resize-disabled";
const char kPageCacheDisabled[] = "page-cache-disabled";
const char kTabToLinksDisabled[] = "tab-to-links-disabled";
const char kHyperlinkAuditingDisabled[] = "hyperlink-auditing-disabled";
const char kUserStyleSheetEnabled[] = "user-style-sheet-enabled";
const char kUserStyleSheetLocation[] = "user-style-sheet-location";
const char kAuthorAndUserStylesDisabled[] = "author-and-user-styles-disabled";
const char kLocalStorageDisabled[] = "local-storage-disabled";
const char kDatabasesDisabled[] = "databases-disabled";
const char kApplicationCacheDisabled[] = "application-cache-disabled";
const char kWebglDisabled[] = "webgl-disabled";
const char kAcceleratedCompositingDisabled[] =
    "accelerated-compositing-disabled";
const char kAcceleratedLayersDisabled[] = "accelerated-layers-disabled";
const char kAcceleratedVideoDisabled[] = "accelerated-video-disabled";
const char kAcceledated2dCanvasDisabled[] = "accelerated-2d-canvas-disabled";
const char kAcceleratedPaintingEnabled[] = "accelerated-painting-enabled";
const char kAcceleratedFiltersEnabled[] = "accelerated-filters-enabled";
const char kAcceleratedPluginsDisabled[] = "accelerated-plugins-disabled";
const char kDeveloperToolsDisabled[] = "developer-tools-disabled";
const char kFullscreenEnabled[] = "fullscreen-enabled";

}  // namespace switches
}  // namespace client
