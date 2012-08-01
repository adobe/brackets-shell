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


@interface ChromeWindowsTerminatedObserver : NSObject
- (void)appTerminated:(NSNotification *)note;
- (void)timeoutTimer:(NSTimer*)timer;
@end


///////////////////////////////////////////////////////////////////////////////
// LiveBrowserMgrMac

class LiveBrowserMgrMac
{
public:
    static LiveBrowserMgrMac* GetInstance();
    static void Shutdown();

    bool IsChromeRunning();
    void CheckForChromeRunning();
    void CheckForChromeRunningTimeout();
    
    void CloseLiveBrowserKillTimers();
    void CloseLiveBrowserFireCallback(int valToSend);

    ChromeWindowsTerminatedObserver* GetTerminateObserver() { return m_chromeTerminateObserver; }
    CefRefPtr<CefV8Value> GetCloseCallback() { return m_closeLiveBrowserCallback; }
    
    void SetCloseTimeoutTimer(NSTimer* closeLiveBrowserTimeoutTimer)
            { m_closeLiveBrowserTimeoutTimer = closeLiveBrowserTimeoutTimer; }
    void SetTerminateObserver(ChromeWindowsTerminatedObserver* chromeTerminateObserver)
            { m_chromeTerminateObserver = chromeTerminateObserver; }
    void SetCloseCallback(CefRefPtr<CefV8Value> closeLiveBrowserCallback)
            { m_closeLiveBrowserCallback = closeLiveBrowserCallback; }
    void SetBrowser(CefRefPtr<CefBrowser> browser)
            { m_browser = browser; }
            

private:
    // private so this class cannot be instantiated externally
    LiveBrowserMgrMac();
    virtual ~LiveBrowserMgrMac();

    NSTimer*                            m_closeLiveBrowserTimeoutTimer;
    CefRefPtr<CefV8Value>               m_closeLiveBrowserCallback;
    CefRefPtr<CefBrowser>               m_browser;
    ChromeWindowsTerminatedObserver*    m_chromeTerminateObserver;
    
    static LiveBrowserMgrMac* s_instance;
};


LiveBrowserMgrMac::LiveBrowserMgrMac()
    : m_closeLiveBrowserTimeoutTimer(nil)
    , m_chromeTerminateObserver(nil)
{
}

LiveBrowserMgrMac::~LiveBrowserMgrMac()
{
    if (s_instance)
        s_instance->CloseLiveBrowserKillTimers();
}

LiveBrowserMgrMac* LiveBrowserMgrMac::GetInstance()
{
    if (!s_instance)
        s_instance = new LiveBrowserMgrMac();
    return s_instance;
}

void LiveBrowserMgrMac::Shutdown()
{
    delete s_instance;
    s_instance = NULL;
}

bool LiveBrowserMgrMac::IsChromeRunning()
{
    NSString *appId = @"com.google.Chrome";
    NSArray *apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:appId];
    for (NSUInteger i = 0; i < apps.count; i++) {
        NSRunningApplication* curApp = [apps objectAtIndex:i];
        if( curApp && !curApp.terminated ) {
            return true;
        }
    }
    return false;
}

void LiveBrowserMgrMac::CloseLiveBrowserKillTimers()
{
    if (m_closeLiveBrowserTimeoutTimer) {
        [m_closeLiveBrowserTimeoutTimer invalidate];
        [m_closeLiveBrowserTimeoutTimer release];
        m_closeLiveBrowserTimeoutTimer = nil;
    }
    
    if (m_chromeTerminateObserver) {
        [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:m_chromeTerminateObserver];
        [m_chromeTerminateObserver release];
        m_chromeTerminateObserver = nil;
    }
}

void LiveBrowserMgrMac::CloseLiveBrowserFireCallback(int valToSend)
{
    // kill the timers
    CloseLiveBrowserKillTimers();
    
    if (!m_closeLiveBrowserCallback.get() || !m_browser.get()) {
        return;
    }
    
    CefRefPtr<CefV8Context> context = m_browser->GetMainFrame()->GetV8Context();
    CefRefPtr<CefV8Value> objectForThis = context->GetGlobal();
    CefV8ValueList args;
    args.push_back( CefV8Value::CreateInt( valToSend ) );
    
    m_closeLiveBrowserCallback->ExecuteFunctionWithContext( context , objectForThis, args );
    
    m_closeLiveBrowserCallback = NULL;
}

void LiveBrowserMgrMac::CheckForChromeRunning()
{
    if (IsChromeRunning())
        return;
    
    CloseLiveBrowserFireCallback(NO_ERROR);
}

void LiveBrowserMgrMac::CheckForChromeRunningTimeout()
{
    int retVal = (IsChromeRunning() ? ERR_UNKNOWN : NO_ERROR);
    
    //notify back to the app
    CloseLiveBrowserFireCallback(retVal);
}

LiveBrowserMgrMac* LiveBrowserMgrMac::s_instance = NULL;

// Forward declarations for functions defined later in this file
void NSArrayToCefList(NSArray* array, CefRefPtr<CefListValue>& list);
int32 ConvertNSErrorCode(NSError* error, bool isReading);

int32 OpenLiveBrowser(ExtensionString argURL, bool enableRemoteDebugging)
{
    // Parse the arguments
    NSString *urlString = [NSString stringWithUTF8String:argURL.c_str()];
    NSURL *url = [NSURL URLWithString:urlString];
    
    // Find instances of the Browser
    NSString *appId = @"com.google.Chrome";
    NSArray *apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:appId];
    NSWorkspace * ws = [NSWorkspace sharedWorkspace];
    NSUInteger launchOptions = NSWorkspaceLaunchDefault | NSWorkspaceLaunchWithoutActivation;

    // Launch Browser
    if(apps.count == 0) {
        
        // Create the configuration dictionary for launching with custom parameters.
        NSArray *parameters = nil;
        if (enableRemoteDebugging) {
            parameters = [NSArray arrayWithObjects:
                           @"--remote-debugging-port=9222", 
                           @"--allow-file-access-from-files",
                           urlString,
                           nil];
        }
        else {
            parameters = [NSArray arrayWithObjects:
                           @"--allow-file-access-from-files",
                           urlString,
                           nil];
        }

        NSMutableDictionary* appConfig = [NSDictionary dictionaryWithObject:parameters forKey:NSWorkspaceLaunchConfigurationArguments];

        NSURL *appURL = [ws URLForApplicationWithBundleIdentifier:appId];
        if( !appURL ) {
            return ERR_NOT_FOUND; //Chrome not installed
        }
        NSError *error = nil;
        if( ![ws launchApplicationAtURL:appURL options:launchOptions configuration:appConfig error:&error] ) {
            return ERR_UNKNOWN;
        }
        return NO_ERROR;
    }
    
    // Tell the Browser to load the url
    [ws openURLs:[NSArray arrayWithObject:url] withAppBundleIdentifier:appId options:launchOptions additionalEventParamDescriptor:nil launchIdentifiers:nil];
    
    return NO_ERROR;
}

int CloseLiveBrowser(CefRefPtr<CefBrowser> browser)
{
    // Reset timeout timer
    LiveBrowserMgrMac* liveBrowserMgr = LiveBrowserMgrMac::GetInstance();
    if (!liveBrowserMgr || !browser.get())
        return ERR_INVALID_PARAMS;
    
    liveBrowserMgr->CloseLiveBrowserKillTimers();
    
    //We can only handle a single async callback at a time. If there is already one that hasn't fired then
    //we kill it now and get ready for the next. 
    liveBrowserMgr->SetBrowser(NULL);
    liveBrowserMgr->SetCloseCallback(NULL);

/***
    if( ! browser->GetMainFrame()->GetV8Context()->IsSame(CefV8Context::GetCurrentContext()) ) {
        ASSERT(FALSE); //Getting called from not the main browser window.
        return ERR_UNKNOWN;
    }
***/
    
    liveBrowserMgr->SetBrowser(browser);
    //liveBrowserMgr->SetCloseCallback(callbackFunction);
    
    // Find instances of the Browser and terminate them
    NSString *appId = @"com.google.Chrome";
    NSArray *apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:appId];
    
    //register an observer to watch for the app terminations
    if( apps.count > 0 && !LiveBrowserMgrMac::GetInstance()->GetTerminateObserver() ) {
        LiveBrowserMgrMac::GetInstance()->SetTerminateObserver([[ChromeWindowsTerminatedObserver alloc] init]);
        
        [[[NSWorkspace sharedWorkspace] notificationCenter]
                addObserver:LiveBrowserMgrMac::GetInstance()->GetTerminateObserver() 
                selector:@selector(appTerminated:) 
                name:NSWorkspaceDidTerminateApplicationNotification 
                object:nil
         ];
    }
    
    for (NSUInteger i = 0; i < apps.count; i++) {
        NSRunningApplication* curApp = [apps objectAtIndex:i];
        if( curApp && !curApp.terminated ) {
            [curApp terminate];
        }
    }
    
    //start a timeout timer
    NSTimeInterval timeoutInSeconds (apps.count == 0 ? 0.0001 : 3 * 60);
    LiveBrowserMgrMac::GetInstance()->SetCloseTimeoutTimer([[NSTimer
                                        scheduledTimerWithTimeInterval:timeoutInSeconds
                                        target:LiveBrowserMgrMac::GetInstance()->GetTerminateObserver()
                                        selector:@selector(timeoutTimer:)
                                        userInfo:nil repeats:NO] retain]
                                    );
    return NO_ERROR;
}

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


void OnBeforeShutdown()
{
    LiveBrowserMgrMac::Shutdown();
}

void CloseWindow(CefRefPtr<CefBrowser> browser)
{
  NSWindow* window = [browser->GetHost()->GetWindowHandle() window];
  
  // Tell the window delegate it's really time to close
  [[window delegate] performSelector:@selector(setIsReallyClosing)];
  browser->GetHost()->CloseBrowser();
}

void BringBrowserWindowToFront(CefRefPtr<CefBrowser> browser)
{
  NSWindow* window = [browser->GetHost()->GetWindowHandle() window];
  [window makeKeyAndOrderFront:nil];
}

@implementation ChromeWindowsTerminatedObserver

- (void) appTerminated:(NSNotification *)note
{
    LiveBrowserMgrMac::GetInstance()->CheckForChromeRunning();
}

- (void) timeoutTimer:(NSTimer*)timer
{
    LiveBrowserMgrMac::GetInstance()->CheckForChromeRunningTimeout();
}

@end

