// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "include/cef_app.h"

#include <CoreFoundation/CoreFoundation.h>

// This file is shared by cefclient and cef_unittests so don't include using
// a qualified path.
#include "client_app.h"  // NOLINT(build/include)

// Application startup time
CFTimeInterval g_appStartupTime;

// Stub implementations.
std::string AppGetWorkingDirectory() {
  return std::string();
}
CefWindowHandle AppGetMainHwnd() {
  return NULL;
}


// Process entry point.
int main(int argc, char* argv[]) {
  CefMainArgs main_args(argc, argv);
	
  g_appStartupTime = CFAbsoluteTimeGetCurrent();

  CefRefPtr<CefApp> app(new ClientApp);

  // Execute the secondary process.
  return CefExecuteProcess(main_args, app);
}
