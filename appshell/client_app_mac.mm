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

#include "client_app.h"

#include <Cocoa/Cocoa.h>

#include <string>

extern CFTimeInterval g_appStartupTime;

double ClientApp::GetElapsedMilliseconds()
{
    CFAbsoluteTime elapsed = CFAbsoluteTimeGetCurrent() - g_appStartupTime;
    
    return round(elapsed * 1000);
}

std::string ClientApp::GetExtensionJSSource()
{
    std::string result;
    
    // This is a hack to grab the extension file from the main app resource bundle.
    // This code may be run in a sub process in an app that is bundled inside the main app.
#if 1
    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    NSString* sourcePath;
    NSRange range = [bundlePath rangeOfString: @"/Frameworks/"];
    
    if (range.location == NSNotFound) {
        sourcePath = [[NSBundle mainBundle] pathForResource: @"appshell_extensions" ofType: @"js"];
    } else {
        sourcePath = [bundlePath substringToIndex:range.location];
        sourcePath = [sourcePath stringByAppendingString:@"/Resources/appshell_extensions.js"];
    }
#else
    NSString* sourcePath = [[NSBundle mainBundle] pathForResource: @"appshell_extensions" ofType: @"js"];
#endif
    
    NSString* jsSource = [[NSString alloc] initWithContentsOfFile:sourcePath encoding:NSUTF8StringEncoding error:nil];
    result = [jsSource UTF8String];
    [jsSource release];
    
    return result;
}
