// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

// Defines all of the command line switches used by cefclient.

#ifndef CEF_TESTS_CEFCLIENT_CEFCLIENT_SWITCHES_H_
#define CEF_TESTS_CEFCLIENT_CEFCLIENT_SWITCHES_H_
#pragma once

namespace cefclient {

extern const char kStartupPath[];

// CefSettings attributes.
extern const char kMultiThreadedMessageLoop[];
extern const char kCachePath[];
extern const char kLogFile[];
extern const char kLogSeverity[];
extern const char kLogSeverity_Verbose[];
extern const char kLogSeverity_Info[];
extern const char kLogSeverity_Warning[];
extern const char kLogSeverity_Error[];
extern const char kLogSeverity_ErrorReport[];
extern const char kLogSeverity_Disable[];
extern const char kGraphicsImpl[];
extern const char kGraphicsImpl_Angle[];
extern const char kGraphicsImpl_AngleCmdBuffer[];
extern const char kGraphicsImpl_Desktop[];
extern const char kGraphicsImpl_DesktopCmdBuffer[];
extern const char kLocalStorageQuota[];
extern const char kSessionStorageQuota[];
extern const char kJavascriptFlags[];

// CefBrowserSettings attributes.
extern const char kDragDropDisabled[];
extern const char kLoadDropsDisabled[];
extern const char kHistoryDisabled[];
extern const char kRemoteFontsDisabled[];
extern const char kDefaultEncoding[];
extern const char kEncodingDetectorEnabled[];
extern const char kJavascriptDisabled[];
extern const char kJavascriptOpenWindowsDisallowed[];
extern const char kJavascriptCloseWindowsDisallowed[];
extern const char kJavascriptAccessClipboardDisallowed[];
extern const char kDomPasteDisabled[];
extern const char kCaretBrowsingDisabled[];
extern const char kJavaDisabled[];
extern const char kPluginsDisabled[];
extern const char kUniversalAccessFromFileUrlsAllowed[];
extern const char kFileAccessFromFileUrlsAllowed[];
extern const char kWebSecurityDisabled[];
extern const char kXssAuditorEnabled[];
extern const char kImageLoadingDisabled[];
extern const char kShrinkStandaloneImagesToFit[];
extern const char kSiteSpecificQuirksDisabled[];
extern const char kTextAreaResizeDisabled[];
extern const char kPageCacheDisabled[];
extern const char kTabToLinksDisabled[];
extern const char kHyperlinkAuditingDisabled[];
extern const char kUserStyleSheetEnabled[];
extern const char kUserStyleSheetLocation[];
extern const char kAuthorAndUserStylesDisabled[];
extern const char kLocalStorageDisabled[];
extern const char kDatabasesDisabled[];
extern const char kApplicationCacheDisabled[];
extern const char kWebglDisabled[];
extern const char kAcceleratedCompositingDisabled[];
extern const char kAcceleratedLayersDisabled[];
extern const char kAcceleratedVideoDisabled[];
extern const char kAcceledated2dCanvasDisabled[];
extern const char kAcceleratedPaintingEnabled[];
extern const char kAcceleratedFiltersEnabled[];
extern const char kAcceleratedPluginsDisabled[];
extern const char kDeveloperToolsDisabled[];
extern const char kFullscreenEnabled[];

}  // namespace cefclient

#endif  // CEF_TESTS_CEFCLIENT_CEFCLIENT_SWITCHES_H_
