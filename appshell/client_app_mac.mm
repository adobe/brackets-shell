#include "client_app.h"

#include <Cocoa/Cocoa.h>

#include <string>

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
        sourcePath = [[NSBundle mainBundle] pathForResource: @"brackets_extensions" ofType: @"js"];
    } else {
        sourcePath = [bundlePath substringToIndex:range.location];
        sourcePath = [sourcePath stringByAppendingString:@"/Resources/brackets_extensions.js"];
    }
#else
    NSString* sourcePath = [[NSBundle mainBundle] pathForResource: @"brackets_extensions" ofType: @"js"];
#endif
    
    NSString* jsSource = [[NSString alloc] initWithContentsOfFile:sourcePath encoding:NSUTF8StringEncoding error:nil];
	result = [jsSource UTF8String];
	[jsSource release];
	
	return result;
}
