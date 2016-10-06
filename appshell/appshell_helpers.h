/*
 * Copyright (c) 2016 - present Adobe Systems Incorporated. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <string>

#include "include/cef_command_line.h"
#include "include/internal/cef_string.h"

#include "config.h"

namespace appshell {

double GetElapsedMilliseconds();
CefString GetCurrentLanguage();
std::string GetExtensionJSSource();
CefString AppGetSupportDirectory();
CefString AppGetDocumentsDirectory();

// Returns the default CEF cache location
CefString AppGetCachePath();

// Returns a string containing the product and version (e.g. "Brackets/0.19.0.0")
CefString AppGetProductVersionString();

// Returns a string containing "Chrome/" appends with its version (e.g. "Chrome/29.0.1547.65")
CefString AppGetChromiumVersionString();

#ifdef OS_LINUX

char* AppInitWorkingDirectory();
std::string AppGetWorkingDirectory();
std::string AppGetRunningDirectory();

int AppInitInitialURL(CefRefPtr<CefCommandLine> command_line);
std::string AppGetInitialURL();

#endif

}  // namespace appshell
