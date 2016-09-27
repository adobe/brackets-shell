// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "appshell/browser/window_test.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "include/base/cef_bind.h"
#include "include/wrapper/cef_stream_resource_handler.h"
#include "appshell/browser/main_message_loop.h"

namespace client {
namespace window_test {

void ModifyBounds(const CefRect& display, CefRect& window) {
  window.x += display.x;
  window.y += display.y;

  if (window.x < display.x)
    window.x = display.x;
  if (window.y < display.y)
    window.y = display.y;
  if (window.width < 100)
    window.width = 100;
  else if (window.width >= display.width)
    window.width = display.width;
  if (window.height < 100)
    window.height = 100;
  else if (window.height >= display.height)
    window.height = display.height;
  if (window.x + window.width >= display.x + display.width)
    window.x = display.x + display.width - window.width;
  if (window.y + window.height >= display.y + display.height)
    window.y = display.y + display.height - window.height;
}

}  // namespace window_test
}  // namespace client
