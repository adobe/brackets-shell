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
#include "include/base/cef_logging.h"
#include "client_handler.h"
#include "appshell/common/client_switches.h"
#include "appshell/appshell_helpers.h"
#include "config.h"

CefRefPtr<ClientHandler> g_handler;

CefWindowHandle AppGetMainHwnd() {
  if (!g_handler.get())
    return NULL;
  return g_handler->GetMainHwnd();
}
