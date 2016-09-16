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
 *
 */

#include "appshell/appshell_versions.h"

#import <Cocoa/Cocoa.h>

#include "include/cef_version.h"
#include "config.h"

namespace appshell {

CefString AppGetProductVersionString() {
  NSMutableString *s = [NSMutableString stringWithString:APP_NAME];
  [s replaceOccurrencesOfString:@" "
                     withString:@""
                        options:NSLiteralSearch
                          range:NSMakeRange(0, [s length])];
  [s appendString:@"/"];
  [s appendString:(NSString*)[[NSBundle mainBundle]
                              objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey]];
  CefString result = CefString([s UTF8String]);
  return result;
}

CefString AppGetChromiumVersionString() {
  NSMutableString *s = [NSMutableString stringWithFormat:@"Chrome/%d.%d.%d.%d",
                           cef_version_info(2), cef_version_info(3),
                           cef_version_info(4), cef_version_info(5)];
  CefString result = CefString([s UTF8String]);
  return result;
}

}  // namespace appshell
