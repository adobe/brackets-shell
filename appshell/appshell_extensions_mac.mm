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

#include "appshell_extensions_platform.h"
#include "appshell_extensions.h"

#include <Cocoa/Cocoa.h>

// Forward declarations for functions defined later in this file
void NSArrayToCefList(NSArray* array, CefRefPtr<CefListValue>& list);
int32 ConvertNSErrorCode(NSError* error, bool isReading);

int32 ShowOpenDialog(bool allowMulitpleSelection,
                     bool chooseDirectory,
                     ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString fileTypes,
                     CefRefPtr<CefListValue>& selectedFiles)
{
    NSArray* allowedFileTypes = nil;
    BOOL canChooseDirectories = chooseDirectory;
    BOOL canChooseFiles = !canChooseDirectories;
    
    if (fileTypes != "")
    {
        // fileTypes is a Space-delimited string
        allowedFileTypes = 
        [[NSString stringWithUTF8String:fileTypes.c_str()] 
         componentsSeparatedByString:@" "];
    }
    
    // Initialize the dialog
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseFiles:canChooseFiles];
    [openPanel setCanChooseDirectories:canChooseDirectories];
    [openPanel setCanCreateDirectories:canChooseDirectories];
    [openPanel setAllowsMultipleSelection:allowMulitpleSelection];
    [openPanel setTitle: [NSString stringWithUTF8String:title.c_str()]];
    
    if (initialDirectory != "")
        [openPanel setDirectoryURL:[NSURL URLWithString:[NSString stringWithUTF8String:initialDirectory.c_str()]]];
    
    [openPanel setAllowedFileTypes:allowedFileTypes];
    
    if ([openPanel runModal] == NSOKButton)
    {
        NSArray* files = [openPanel filenames];
        NSArrayToCefList(files, selectedFiles);
    }
    
    return NO_ERROR;
}

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents)
{
    NSString* pathStr = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    
    NSArray* contents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:pathStr error:&error];
    
    if (contents != nil)
    {
        NSArrayToCefList(contents, directoryContents);
        return NO_ERROR; 
    }
    
    return ConvertNSErrorCode(error, true);
}

int32 GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir)
{
    NSString* path = [NSString stringWithUTF8String:filename.c_str()];
    BOOL isDirectory;
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory]) {
        isDir = isDirectory;
    } else {
        return ERR_NOT_FOUND;
    }
    
    NSError* error = nil;
    NSDictionary* fileAttribs = [[NSFileManager defaultManager] attributesOfItemAtPath:path error:&error];
    NSDate *modDate = [fileAttribs valueForKey:NSFileModificationDate];
    modtime = [modDate timeIntervalSince1970];
    
    return ConvertNSErrorCode(error, true);
}

int32 ReadFile(ExtensionString filename, ExtensionString encoding, std::string& contents)
{
    NSString* path = [NSString stringWithUTF8String:filename.c_str()];
    
    NSStringEncoding enc;
    NSError* error = nil;
    
    if (encoding == "utf8")
        enc = NSUTF8StringEncoding;
    else
        return ERR_UNSUPPORTED_ENCODING; 
    
    NSString* fileContents = [NSString stringWithContentsOfFile:path encoding:enc error:&error];
    
    if (fileContents) 
    {
        contents = [fileContents UTF8String];
        return NO_ERROR;
    }
    
    return ConvertNSErrorCode(error, true);
}

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding)
{
    NSString* path = [NSString stringWithUTF8String:filename.c_str()];
    NSString* contentsStr = [NSString stringWithUTF8String:contents.c_str()];
    NSStringEncoding enc;
    NSError* error = nil;
    
    if (encoding == "utf8")
        enc = NSUTF8StringEncoding;
    else
        return ERR_UNSUPPORTED_ENCODING;
    
    const NSData* encodedContents = [contentsStr dataUsingEncoding:enc];
    NSUInteger len = [encodedContents length];
    NSOutputStream* oStream = [NSOutputStream outputStreamToFileAtPath:path append:NO];
    
    [oStream open];
    NSInteger res = [oStream write:(const uint8_t*)[encodedContents bytes] maxLength:len];
    [oStream close];
    
    if (res == -1) {
        error = [oStream streamError];
    }  
    
    return ConvertNSErrorCode(error, false);
}

int32 SetPosixPermissions(ExtensionString filename, int32 mode)
{
    NSError* error = nil;
    
    NSString* path = [NSString stringWithUTF8String:filename.c_str()];
    NSDictionary* attrs = [NSDictionary dictionaryWithObject:[NSNumber numberWithInt:mode] forKey:NSFilePosixPermissions];
    
    if ([[NSFileManager defaultManager] setAttributes:attrs ofItemAtPath:path error:&error])
        return NO_ERROR;
    
    return ConvertNSErrorCode(error, false);
}

int32 DeleteFileOrDirectory(ExtensionString filename)
{
    NSError* error = nil;
    
    NSString* path = [NSString stringWithUTF8String:filename.c_str()];
    BOOL isDirectory;
    
    // Contrary to the name of this function, we don't actually delete directories
    if ([[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory]) {
        if (isDirectory) {
            return ERR_NOT_FILE;
        }
    } else {
        return ERR_NOT_FOUND;
    }    
    
    if ([[NSFileManager defaultManager] removeItemAtPath:path error:&error])
        return NO_ERROR;
    
    return ConvertNSErrorCode(error, false);
}

void CloseWindow(CefRefPtr<CefBrowser> browser)
{
  NSWindow* window = [browser->GetHost()->GetWindowHandle() window];
  
  // Try to make the window go away.
  [window autorelease];
  
  // Clean ourselves up after clearing the stack of anything that might have the
  // window on it.
  [(NSObject*)[window delegate] performSelectorOnMainThread:@selector(cleanup:)
                                                 withObject:window  waitUntilDone:NO];
}

void NSArrayToCefList(NSArray* array, CefRefPtr<CefListValue>& list)
{
    for (NSUInteger i = 0; i < [array count]; i++) {
        list->SetString(i, [[array objectAtIndex:i] UTF8String]);
    }
}

int32 ConvertNSErrorCode(NSError* error, bool isReading)
{
    if (!error)
        return NO_ERROR;
    
    if( [[error domain] isEqualToString: NSPOSIXErrorDomain] )
    {
        switch ([error code]) 
        {
            case ENOENT:
                return ERR_NOT_FOUND;
                break;
            case EPERM:
            case EACCES:
                return (isReading ? ERR_CANT_READ : ERR_CANT_WRITE);
                break;
            case EROFS:
                return ERR_CANT_WRITE;
                break;
            case ENOSPC:
                return ERR_OUT_OF_SPACE;
                break;
        }
        
    }
    
    
    switch ([error code]) 
    {
        case NSFileNoSuchFileError:
        case NSFileReadNoSuchFileError:
            return ERR_NOT_FOUND;
            break;
        case NSFileReadNoPermissionError:
            return ERR_CANT_READ;
            break;
        case NSFileReadInapplicableStringEncodingError:
            return ERR_UNSUPPORTED_ENCODING;
            break;
        case NSFileWriteNoPermissionError:
            return ERR_CANT_WRITE;
            break;
        case NSFileWriteOutOfSpaceError:
            return ERR_OUT_OF_SPACE;
            break;
    }
    
    // Unknown error
    return ERR_UNKNOWN;
}

