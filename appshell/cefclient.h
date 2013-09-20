// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_CEFCLIENT_H_
#define CEF_TESTS_CEFCLIENT_CEFCLIENT_H_
#pragma once

#include <string>
#include "include/cef_base.h"
#include "client_app.h"

class CefApp;
class CefBrowser;
class CefCommandLine;

// Returns the main browser window instance.
CefRefPtr<CefBrowser> AppGetBrowser();

// Returns the main application window handle.
CefWindowHandle AppGetMainHwnd();

// Returns the application working directory.
std::string AppGetWorkingDirectory();

// Returns the starting URL
CefString AppGetInitialURL();


// Returns the default CEF cache location
CefString AppGetCachePath();

// Returns a string containing the product and version (e.g. "Brackets/0.19.0.0")
CefString AppGetProductVersionString();

// Returns a string containing "Chrome/" appends with its version (e.g. "Chrome/29.0.1547.65")
CefString AppGetChromiumVersionString();

// Initialize the application command line.
void AppInitCommandLine(int argc, const char* const* argv);

// Returns the application command line object.
CefRefPtr<CefCommandLine> AppGetCommandLine();

// Returns the application settings based on command line arguments.
void AppGetSettings(CefSettings& settings, CefRefPtr<ClientApp> app);

// Returns the application browser settings based on command line arguments.
void AppGetBrowserSettings(CefBrowserSettings& settings);

#endif  // CEF_TESTS_CEFCLIENT_CEFCLIENT_H_
