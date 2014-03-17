/*
 * Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
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
 *
 */

#pragma once

// Application name used in native code. This name is *not* used in resources.

#ifdef OS_WIN
// MAX_PATH is only 260 chars which really isn't big enough for really long unc pathnames
//  so use this constant instead which accounts for some really long pathnames
#define MAX_UNC_PATH 4096
// Name of group (if any) that application prefs/settings/etc. are stored under
// This must be an empty string (for no group), or a string that ends with "\\"
#define GROUP_NAME L""
#define APP_NAME L"Brackets"
#define WINDOW_TITLE APP_NAME

// Paths for node resources are relative to the location of the appshell executable
#define NODE_EXECUTABLE_PATH "node.exe"
#define NODE_CORE_PATH "node-core"
#define FIRST_INSTANCE_MUTEX_NAME	(APP_NAME L".Shell.Instance")
#endif
#ifdef OS_MACOSX
// Name of group (if any) that application prefs/settings/etc. are stored under
// This must be an empty string (for no group), or a string that ends with "/"
#define GROUP_NAME @""
#define APP_NAME @"Brackets"
#define WINDOW_TITLE APP_NAME

// Paths for node resources are relative to the bundle path
#define NODE_EXECUTABLE_PATH @"/Contents/MacOS/Brackets-node"
#define NODE_CORE_PATH @"/Contents/node-core"

#endif
#ifdef OS_LINUX
// TODO linux preferences
//#define GROUP_NAME @""
#define APP_NAME "Brackets"
//#define WINDOW_TITLE APP_NAME

// Path for node resources is in dependencies dir and relative to the location of the appshell executable
#define NODE_EXECUTABLE_PATH "Brackets-node"
#define NODE_CORE_PATH "node-core"

#endif

#define REMOTE_DEBUGGING_PORT 9234

// Comment out this line to enable OS themed drawing
#define DARK_UI 
#define DARK_AERO_GLASS
#define CUSTOM_TRAFFIC_LIGHTS
#define LIGHT_CAPTION_TEXT

