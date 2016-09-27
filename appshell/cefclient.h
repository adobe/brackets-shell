// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_CEFCLIENT_H_
#define CEF_TESTS_CEFCLIENT_CEFCLIENT_H_
#pragma once

#include <string>
#include "include/cef_base.h"
#include "appshell/common/client_app.h"
#include "appshell/config.h"

class CefApp;
class CefBrowser;
class CefCommandLine;

#if defined(OS_MACOSX)

// Returns the main application window handle.
CefWindowHandle AppGetMainHwnd();

#endif

// Returns the application working directory.
std::string AppGetWorkingDirectory();

#endif  // CEF_TESTS_CEFCLIENT_CEFCLIENT_H_
