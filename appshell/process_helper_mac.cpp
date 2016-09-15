// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "include/cef_app.h"

#include <CoreFoundation/CoreFoundation.h>

#include "appshell/common/client_app_other.h"
#include "appshell/renderer/client_app_renderer.h"

// Application startup time
CFTimeInterval g_appStartupTime;

// Stub implementations.
std::string AppGetWorkingDirectory() {
  return std::string();
}
CefWindowHandle AppGetMainHwnd() {
  return NULL;
}

namespace client {

int RunMain(int argc, char* argv[]) {
  CefMainArgs main_args(argc, argv);

  g_appStartupTime = CFAbsoluteTimeGetCurrent();

  // Parse command-line arguments.
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromArgv(argc, argv);

  // Create a ClientApp of the correct type.
  CefRefPtr<CefApp> app;
  ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);
  if (process_type == ClientApp::RendererProcess)
    app = new ClientAppRenderer();
  else if (process_type == ClientApp::OtherProcess)
    app = new ClientAppOther();

  // Execute the secondary process.
  return CefExecuteProcess(main_args, app, NULL);
}

}  // namespace client


// Process entry point.
int main(int argc, char* argv[]) {
  return client::RunMain(argc, argv);
}
