/*
 * Copyright (c) 2012 - present Adobe Systems Incorporated. All rights reserved.
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

#include "appshell/appshell_helpers.h"
#include "native_menu_model.h"

#include "GoogleChrome.h"

#include <Cocoa/Cocoa.h>
#include <sys/sysctl.h>

#include <sstream>
#include <unicode/ucsdet.h>
#include <unicode/ucnv.h>
#include <fstream>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/if_types.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <mach-o/arch.h>

#define UTF8_BOM "\xEF\xBB\xBF"

NSMutableArray* pendingOpenFiles;

@interface ChromeWindowsTerminatedObserver : NSObject
- (void)appTerminated:(NSNotification *)note;
- (void)timeoutTimer:(NSTimer*)timer;
@end

// LiveBrowser helper functions
NSRunningApplication* GetLiveBrowserApp(NSString *bundleId, int debugPort);

// App ID for either Chrome or Chrome Canary (commented out)
NSString *const appId = @"com.google.Chrome";
//NSString *const appId = @"com.google.Chrome.canary";

// Live Development browser debug paramaters
int const debugPort = 9222;
NSString* debugPortCommandlineArguments = [NSString stringWithFormat:@"--remote-debugging-port=%d", debugPort];
NSString* debugProfilePath = [NSString stringWithFormat:@"--user-data-dir=%s/live-dev-profile", appshell::AppGetSupportDirectory().ToString().c_str()];

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
    
    void SetWorkspaceNotifications();
    void RemoveWorkspaceNotifications();

    void CloseLiveBrowserKillTimers();
    void CloseLiveBrowserFireCallback(int valToSend);

    ChromeWindowsTerminatedObserver* GetTerminateObserver() { return m_chromeTerminateObserver; }
    CefRefPtr<CefProcessMessage> GetCloseCallback() { return m_closeLiveBrowserCallback; }
    NSRunningApplication* GetLiveBrowser() { return GetLiveBrowserApp(appId, debugPort); }
    int GetLiveBrowserPid() { return m_liveBrowserPid; }
    
    void SetCloseTimeoutTimer(NSTimer* closeLiveBrowserTimeoutTimer)
            { m_closeLiveBrowserTimeoutTimer = closeLiveBrowserTimeoutTimer; }
    void SetTerminateObserver(ChromeWindowsTerminatedObserver* chromeTerminateObserver)
            { m_chromeTerminateObserver = chromeTerminateObserver; }
    void SetCloseCallback(CefRefPtr<CefProcessMessage> response)
            { m_closeLiveBrowserCallback = response; }
    void SetBrowser(CefRefPtr<CefBrowser> browser)
            { m_browser = browser; }
    void SetLiveBrowserPid(int pid)
            { m_liveBrowserPid = pid; }

private:
    // private so this class cannot be instantiated externally
    LiveBrowserMgrMac();
    virtual ~LiveBrowserMgrMac();

    NSTimer*                            m_closeLiveBrowserTimeoutTimer;
    CefRefPtr<CefProcessMessage>        m_closeLiveBrowserCallback;
    CefRefPtr<CefBrowser>               m_browser;
    ChromeWindowsTerminatedObserver*    m_chromeTerminateObserver;
    int                                 m_liveBrowserPid;
    
    static LiveBrowserMgrMac*           s_instance;
};


LiveBrowserMgrMac::LiveBrowserMgrMac()
    : m_closeLiveBrowserTimeoutTimer(nil)
    , m_chromeTerminateObserver(nil)
    , m_liveBrowserPid(ERR_PID_NOT_FOUND)
{
}

LiveBrowserMgrMac::~LiveBrowserMgrMac()
{
    if (s_instance)
        s_instance->CloseLiveBrowserKillTimers();

    RemoveWorkspaceNotifications();
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
    return GetLiveBrowser() ? true : false;
}

void LiveBrowserMgrMac::CloseLiveBrowserKillTimers()
{
    if (m_closeLiveBrowserTimeoutTimer) {
        [m_closeLiveBrowserTimeoutTimer invalidate];
        [m_closeLiveBrowserTimeoutTimer release];
        m_closeLiveBrowserTimeoutTimer = nil;
    }
}

void LiveBrowserMgrMac::CloseLiveBrowserFireCallback(int valToSend)
{
    // kill the timers
    CloseLiveBrowserKillTimers();

    // Stop listening for ws shutdown notifications
    RemoveWorkspaceNotifications();

    // Prepare response
    if (m_closeLiveBrowserCallback && m_browser) {

        CefRefPtr<CefListValue> responseArgs = m_closeLiveBrowserCallback->GetArgumentList();

        // Set common response args (callbackId and error)
        responseArgs->SetInt(1, valToSend);

        // Send response
        m_browser->SendProcessMessage(PID_RENDERER, m_closeLiveBrowserCallback);
    }
    
    // Clear state
    m_closeLiveBrowserCallback = NULL;
    m_browser = NULL;
}

void LiveBrowserMgrMac::CheckForChromeRunning()
{
    if (IsChromeRunning())
        return;
    
    // Unset the LiveBrowser pid
    m_liveBrowserPid = ERR_PID_NOT_FOUND;

    // Fire callback to browser
    CloseLiveBrowserFireCallback(NO_ERROR);
}

void LiveBrowserMgrMac::CheckForChromeRunningTimeout()
{
    int retVal = (IsChromeRunning() ? ERR_UNKNOWN : NO_ERROR);
    
    //notify back to the app
    CloseLiveBrowserFireCallback(retVal);
}

void LiveBrowserMgrMac::SetWorkspaceNotifications()
{
    if (!GetTerminateObserver()) {
        //register an observer to watch for the app terminations
        SetTerminateObserver([[ChromeWindowsTerminatedObserver alloc] init]);

        [[[NSWorkspace sharedWorkspace] notificationCenter]
         addObserver:GetTerminateObserver()
         selector:@selector(appTerminated:)
         name:NSWorkspaceDidTerminateApplicationNotification
         object:nil
         ];
    }
}

void LiveBrowserMgrMac::RemoveWorkspaceNotifications()
{
    if (m_chromeTerminateObserver) {
        [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:m_chromeTerminateObserver];
        [m_chromeTerminateObserver release];
        m_chromeTerminateObserver = nil;
    }
}

LiveBrowserMgrMac* LiveBrowserMgrMac::s_instance = NULL;

// Forward declarations for functions defined later in this file
void NSArrayToCefList(NSArray* array, CefRefPtr<CefListValue>& list);
int32 ConvertNSErrorCode(NSError* error, bool isReading);
GoogleChromeApplication* GetGoogleChromeApplicationWithPid(int PID)
{
    try {
        // Ensure we have a valid process id before invoking ScriptingBridge.
        // We need this because negative pids (e.g ERR_PID_NOT_FOUND) will not
        // throw an exception, but rather will return a non-nil junk object
        // that causes Brackets to hang on close
        GoogleChromeApplication* app = PID < 0 ? nil : [SBApplication applicationWithProcessIdentifier:PID];

        // Second check before returning
        return [app respondsToSelector:@selector(name)] && [app.name isEqualToString:@"Google Chrome"] ? app : nil;
    }
    catch (...) {
        return nil;
    }
}

int32 OpenLiveBrowser(ExtensionString argURL, bool enableRemoteDebugging)
{
    LiveBrowserMgrMac* liveBrowserMgr = LiveBrowserMgrMac::GetInstance();

    // Parse the arguments
    NSString *urlString = [NSString stringWithUTF8String:argURL.c_str()];
    
    // Find instances of the Browser
    NSRunningApplication* liveBrowser = liveBrowserMgr->GetLiveBrowser();
    
    // Get the corresponding chromeApp scriptable browser object
    GoogleChromeApplication* chromeApp = !liveBrowser ? nil : GetGoogleChromeApplicationWithPid([liveBrowser processIdentifier]);

    // Launch Browser
    if (!chromeApp) {
        NSURL* appURL = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:appId];
        if( !appURL ) {
            return ERR_NOT_FOUND; //Chrome not installed
        }

        // Create the configuration dictionary for launching with custom parameters.
        NSArray *parameters = [NSArray arrayWithObjects:
                      @"--no-first-run",
                      @"--no-default-browser-check",
                      @"--disable-default-apps",
                      debugPortCommandlineArguments,
                      debugProfilePath,
                      @"--disk-cache-size=250000000",
                      urlString,
                      nil];

        NSDictionary* appConfig = [NSDictionary dictionaryWithObject:parameters forKey:NSWorkspaceLaunchConfigurationArguments];
        NSUInteger launchOptions = NSWorkspaceLaunchDefault | NSWorkspaceLaunchNewInstance;

        liveBrowser = [[NSWorkspace sharedWorkspace] launchApplicationAtURL:appURL options:launchOptions configuration:appConfig error:nil];
        if (!liveBrowser) {
            return ERR_UNKNOWN;
        }

        liveBrowserMgr->SetLiveBrowserPid([liveBrowser processIdentifier]);
        liveBrowserMgr->SetWorkspaceNotifications();

        return NO_ERROR;
    }

    [liveBrowser activateWithOptions:NSApplicationActivateIgnoringOtherApps];

    // Check for existing tab with url already loaded
    for (GoogleChromeWindow* chromeWindow in [chromeApp windows]) {
        for (GoogleChromeTab* tab in [chromeWindow tabs]) {
            if ([tab.URL isEqualToString:urlString]) {
                // Found and open tab with url already loaded
                return NO_ERROR;
            }
        }
    }

    // Tell the Browser to load the url
    GoogleChromeWindow* chromeWindow = [[chromeApp windows] objectAtIndex:0];
    if (!chromeWindow || [[chromeWindow tabs] count] == 0) {
        // Create new Window
        GoogleChromeWindow* chromeWindow = [[[chromeApp classForScriptingClass:@"window"] alloc] init];
        [[chromeApp windows] addObject:chromeWindow];
        chromeWindow.activeTab.URL = urlString;
        [chromeWindow release];
    } else {
        // Create new Tab
        GoogleChromeTab* chromeTab = [[[chromeApp classForScriptingClass:@"tab"] alloc] initWithProperties:@{@"URL": urlString}];
        [[chromeWindow tabs] addObject:chromeTab];
        [chromeTab release];
    }

    return NO_ERROR;
}

void CloseLiveBrowser(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    LiveBrowserMgrMac* liveBrowserMgr = LiveBrowserMgrMac::GetInstance();
    
    if (liveBrowserMgr->GetCloseCallback() != NULL) {
        // We can only handle a single async callback at a time. If there is already one that hasn't fired then
        // we kill it now and get ready for the next.
        liveBrowserMgr->CloseLiveBrowserFireCallback(ERR_UNKNOWN);
    }
    
    // Set up new Brackets CloseLiveBrowser callbacks
    liveBrowserMgr->SetBrowser(browser);
    liveBrowserMgr->SetCloseCallback(response);
    
    // Get the currently active LiveBrowser session
    NSRunningApplication* liveBrowser = liveBrowserMgr->GetLiveBrowser();
    if (!liveBrowser) {
        // No active LiveBrowser found
        liveBrowserMgr->CloseLiveBrowserFireCallback(NO_ERROR);
        return;
    }

    GoogleChromeApplication* chromeApp = GetGoogleChromeApplicationWithPid([liveBrowser processIdentifier]);
    if (!chromeApp) {
        // No corresponding scriptable browser object found
        liveBrowserMgr->CloseLiveBrowserFireCallback(NO_ERROR);
        return;
    }

    // Technically at this point we would locate the LiveBrowser window and
    // close all tabs; however, the LiveDocument tab was already closed by Inspector!
    // and there is no way to find which window to close.

    // Do not close other windows
    if ([[chromeApp windows] count] > 0 || [[[[chromeApp windows] objectAtIndex:0] tabs] count] > 0) {
        liveBrowserMgr->CloseLiveBrowserFireCallback(NO_ERROR);
        return;
    }
    
    // Set up workspace shutdown notifications
    liveBrowserMgr->SetLiveBrowserPid([liveBrowser processIdentifier]);
    liveBrowserMgr->SetWorkspaceNotifications();

    // No more open windows found, so quit Chrome
    [chromeApp quit];

    // Set timeout timer
    liveBrowserMgr->SetCloseTimeoutTimer([[NSTimer
                                         scheduledTimerWithTimeInterval:(3 * 60)
                                         target:liveBrowserMgr->GetTerminateObserver()
                                         selector:@selector(timeoutTimer:)
                                         userInfo:nil repeats:NO] retain]
                                         );
}

int32 OpenURLInDefaultBrowser(ExtensionString url)
{
    NSString* urlString = [NSString stringWithUTF8String:url.c_str()];
    
    if ([[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: urlString]] == NO) {
        return ERR_UNKNOWN;
    }
    
    return NO_ERROR;
}

void ShowOpenDialog(bool allowMulitpleSelection,
                     bool chooseDirectory,
                     ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString fileTypes,
                     CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefProcessMessage> response)
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
    [openPanel setShowsHiddenFiles: YES];
    [openPanel setTitle: [NSString stringWithUTF8String:title.c_str()]];
    
    if (initialDirectory != "")
        [openPanel setDirectoryURL:[NSURL URLWithString:[NSString stringWithUTF8String:initialDirectory.c_str()]]];
    
    [openPanel setAllowedFileTypes:allowedFileTypes];
    
    // cache the browser and response variables, so that these
    // can be accessed from within the completionHandler block.
    CefRefPtr<CefBrowser>        _browser  = browser;
    CefRefPtr<CefProcessMessage> _response = response;

    [openPanel beginSheetModalForWindow:[NSApp mainWindow] completionHandler: ^(NSInteger returnCode)
    {
        if(_browser && _response){

            NSArray *urls = [openPanel URLs];
            CefRefPtr<CefListValue> selectedFiles = CefListValue::Create();
            if (returnCode == NSModalResponseOK){
                for (NSUInteger i = 0; i < [urls count]; i++) {
                    selectedFiles->SetString(i, [[[urls objectAtIndex:i] path] UTF8String]);
                }
            }

            // Set common response args (error and selectedfiles list)
            _response->GetArgumentList()->SetInt(1, NO_ERROR);
            _response->GetArgumentList()->SetList(2, selectedFiles);
            _browser->SendProcessMessage(PID_RENDERER, _response);
        }
    }];
}

void ShowSaveDialog(ExtensionString title,
                       ExtensionString initialDirectory,
                       ExtensionString proposedNewFilename,
                       CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefProcessMessage> response)
{
    NSSavePanel* savePanel = [NSSavePanel savePanel];
    [savePanel setTitle: [NSString stringWithUTF8String:title.c_str()]];
    
    if (initialDirectory != "")
    {
        NSURL* initialDir = [NSURL fileURLWithPath:[NSString stringWithUTF8String:initialDirectory.c_str()]];
        [savePanel setDirectoryURL:initialDir];
    }

    [savePanel setNameFieldStringValue:[NSString stringWithUTF8String:proposedNewFilename.c_str()]];

    // cache the browser and response variables, so that these
    // can be accessed from within the completionHandler block.
    CefRefPtr<CefBrowser>        _browser  = browser;
    CefRefPtr<CefProcessMessage> _response = response;

    [savePanel beginSheetModalForWindow:[NSApp mainWindow] completionHandler: ^(NSInteger returnCode)
    {
         if(_response && _browser){

             CefString pathStr;
             if (returnCode == NSModalResponseOK){
                 NSURL* selectedFile = [savePanel URL];
                 if(selectedFile)
                     pathStr = [[selectedFile path] UTF8String];
             }

             // Set common response args (error and the new file name string)
             _response->GetArgumentList()->SetInt(1, NO_ERROR);
             _response->GetArgumentList()->SetString(2, pathStr);
             _browser->SendProcessMessage(PID_RENDERER, _response);
         }
    }];
}

int32 IsNetworkDrive(ExtensionString path, bool& isRemote)
{
    NSString* pathStr = [NSString stringWithUTF8String:path.c_str()];
    isRemote = false;
    
    if ([pathStr length] == 0) {
        return ERR_INVALID_PARAMS;
    }

    // Detect remote drive
    NSString *testPath = [[pathStr copy] autorelease];
    NSNumber *isVolumeKey;
    NSError *error = nil;

    while (![testPath isEqualToString:@"/"]) {
        NSURL *testUrl = [NSURL fileURLWithPath:testPath];

        if (![testUrl getResourceValue:&isVolumeKey forKey:NSURLIsVolumeKey error:&error]) {
            return ERR_NOT_FOUND;
        }
        if ([isVolumeKey boolValue]) {
            isRemote = true;
            break;
        }
        testPath = [testPath stringByDeletingLastPathComponent];
    }

    return NO_ERROR;
}

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents)
{
    NSString* pathStr = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    
    if ([pathStr length] == 0) {
        return ERR_INVALID_PARAMS;
    }

    NSArray* contents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:pathStr error:&error];
    
    if (contents != nil)
    {
        NSArrayToCefList(contents, directoryContents);
        return NO_ERROR; 
    }
    
    return ConvertNSErrorCode(error, true);
}

int32 MakeDir(ExtensionString path, int32 mode)
{
    NSError* error = nil;
    NSString* pathStr = [NSString stringWithUTF8String:path.c_str()];
  
    // TODO (issue #1759): honor mode
    [[NSFileManager defaultManager] createDirectoryAtPath:pathStr withIntermediateDirectories:TRUE attributes:nil error:&error];
  
    return ConvertNSErrorCode(error, false);
}

// perform a case insensitive filename comparison
int32 compareCaseInsensitive(std::string str1, std::string str2)
{
    std::transform(str1.begin(), str1.end(), str1.begin(), toupper);
    std::transform(str2.begin(), str2.end(), str2.begin(), toupper);
    return str1.compare(str2);
}

int32 Rename(ExtensionString oldName, ExtensionString newName)
{
    NSError* error = nil;
    NSString* oldPathStr = [NSString stringWithUTF8String:oldName.c_str()];
    NSString* newPathStr = [NSString stringWithUTF8String:newName.c_str()];
    
    // check if the filename change is a case-only change
    if (compareCaseInsensitive(oldName, newName) != 0) {
        // Check to make sure newName doesn't already exist. On OS 10.7 and later, moveItemAtPath
        // returns a nice "NSFileWriteFileExists" error in this case, but 10.6 returns a generic
        // "can't write" error.
        if ([[NSFileManager defaultManager] fileExistsAtPath:newPathStr]) {
            return ERR_FILE_EXISTS;
        }
        
        [[NSFileManager defaultManager] moveItemAtPath:oldPathStr toPath:newPathStr error:&error];
    } else {
        // brackets issue #8127 - must rename case-only filename changes using an intermediate
        //   temp filename.  Otherwise, NSFileManager -moveItemAtPath fails.
        ExtensionString tmpName;
        NSString* tmpPathStr = NULL;
        
        // find an intermediate filename that doesn't already exist
        int idx = 0;
        std::ostringstream buff("");
        do {
            buff.str("");
            buff << idx++;
            tmpName = newName + "." + buff.str();
            tmpPathStr = [NSString stringWithUTF8String:tmpName.c_str()];
        } while ([[NSFileManager defaultManager] fileExistsAtPath:tmpPathStr]);
        
        if ([[NSFileManager defaultManager] moveItemAtPath:oldPathStr toPath:tmpPathStr error:&error]) {
            if (![[NSFileManager defaultManager] moveItemAtPath:tmpPathStr toPath:newPathStr error:&error]) {
                // recover if can't move to final destination
                [[NSFileManager defaultManager] moveItemAtPath:tmpPathStr toPath:oldPathStr error:&error];
            }
        }
    }
  
    return ConvertNSErrorCode(error, false);
}

int32 GetFileInfo(ExtensionString filename, uint32& modtime, bool& isDir, double& size, ExtensionString& realPath)
{
    NSString* path = [NSString stringWithUTF8String:filename.c_str()];
    BOOL isDirectory;
    
    // Strip trailing "/"
    if ([path hasSuffix:@"/"] && [path length] > 1) {
        path = [path substringToIndex:[path length] - 1];
    }
    if ([[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory]) {
        isDir = isDirectory;
    } else {
        return ERR_NOT_FOUND;
    }
    
    NSError* error = nil;
    NSDictionary* fileAttribs = [[NSFileManager defaultManager] attributesOfItemAtPath:path error:&error];
    
    // If path is a symlink, resolve it here and get the attributes at the
    // resolved path
    if ([[fileAttribs fileType] isEqualToString:NSFileTypeSymbolicLink]) {
        NSString* realPathStr = [path stringByResolvingSymlinksInPath];
        realPath = [realPathStr UTF8String];
        fileAttribs = [[NSFileManager defaultManager] attributesOfItemAtPath:realPathStr error:&error];
    } else {
        realPath = "";
    }
    
    NSDate *modDate = [fileAttribs valueForKey:NSFileModificationDate];
    modtime = [modDate timeIntervalSince1970];
    NSNumber *filesize = [fileAttribs valueForKey:NSFileSize];
    size = [filesize doubleValue];
    return ConvertNSErrorCode(error, true);
}

int32 ReadFile(ExtensionString filename, ExtensionString& encoding, std::string& contents, bool& preserveBOM)
{
    if (encoding == "utf8") {
        encoding = "UTF-8";
    }
    NSString* path = [NSString stringWithUTF8String:filename.c_str()];
    
    NSStringEncoding enc;
    int32 error = NO_ERROR;
    
    NSString* fileContents = nil;
    if (encoding == "UTF-8") {
        enc = NSUTF8StringEncoding;
        NSError* NSerror = nil;
        fileContents = [NSString stringWithContentsOfFile:path encoding:enc error:&NSerror];
    }
    
    if (fileContents)
    {
        contents = [fileContents UTF8String];
        // We check if the file contains BOM or not
        // if yes, then we set preserveBOM to true
        // Please note we try to read first 3 characters
        // again to check for BOM
        CheckForUTF8BOM(filename, preserveBOM);
        return NO_ERROR;
    } else {
        try {
            std::ifstream file(filename.c_str());
            std::stringstream ss;
            ss << file.rdbuf();
            contents = ss.str();
            std::string detectedCharSet;
            try {
                if (encoding == "UTF-8") {
                    CharSetDetect ICUDetector;
                    ICUDetector(contents.c_str(), contents.size(), detectedCharSet);
                }
                else {
                    detectedCharSet = encoding;
                }
                if (detectedCharSet == "UTF-16LE" || detectedCharSet == "UTF-16BE") {
                    return ERR_UNSUPPORTED_UTF16_ENCODING;
                }
                if (!detectedCharSet.empty()) {
                    std::transform(detectedCharSet.begin(), detectedCharSet.end(), detectedCharSet.begin(), ::toupper);
                    DecodeContents(contents, detectedCharSet);
                    encoding = detectedCharSet;
                }
                else {
                    error = ERR_UNSUPPORTED_ENCODING;
                }
            } catch (...) {
                error = ERR_UNSUPPORTED_ENCODING;
            }
        } catch (...) {
            error = ERR_CANT_READ;
        }
    }
    
    return error;}

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding, bool preserveBOM)
{
    const char *filenameStr = filename.c_str();
    int32 error = NO_ERROR;
    if (encoding == "utf8") {
        encoding = "UTF-8";
    }
    
    if (encoding != "UTF-8") {
        try {
            CharSetEncode ICUEncoder(encoding);
            ICUEncoder(contents);
        } catch (...) {
            error = ERR_ENCODE_FILE_FAILED;
        }
    } else if (encoding == "UTF-8" && preserveBOM) {
        // The file originally contained BOM chars
        // so we prepend BOM chars
        contents = UTF8_BOM + contents;
    }
    
    try {
        std::ofstream file;
        file.open (filenameStr);
        file << contents;
        if (file.fail()) {
            error = ERR_CANT_WRITE;
        }
        file.close();
    } catch (...) {
        return ERR_CANT_WRITE;
    }
    
    return error;
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
    
    // Make sure it exists
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) {
        return ERR_NOT_FOUND;
    }
        
    if ([[NSFileManager defaultManager] removeItemAtPath:path error:&error])
        return NO_ERROR;       
    
    return ConvertNSErrorCode(error, false);
}

void MoveFileOrDirectoryToTrash(ExtensionString filename, CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    NSString* pathStr = [NSString stringWithUTF8String:filename.c_str()];
    NSURL* fileUrl = [NSURL fileURLWithPath: pathStr];
    
    static CefRefPtr<CefProcessMessage> s_response;
    static CefRefPtr<CefBrowser> s_browser;
    
    if (s_response) {
        // Already a pending request. This will only happen if MoveFileOrDirectoryToTrash is called
        // before the previous call has completed, which is not very likely.
        response->GetArgumentList()->SetInt(1, ERR_UNKNOWN);
        browser->SendProcessMessage(PID_RENDERER, response);
        return;
    }
    
    s_browser = browser;
    s_response = response;
    
    [[NSWorkspace sharedWorkspace] recycleURLs:[NSArray arrayWithObject:fileUrl] completionHandler:^(NSDictionary *newURLs, NSError *error) {
        // Invoke callback
        s_response->GetArgumentList()->SetInt(1, ConvertNSErrorCode(error, false));
        s_browser->SendProcessMessage(PID_RENDERER, s_response);
        
        s_response = nil;
        s_browser = nil;
    }];
}

int32 CopyFile(ExtensionString src, ExtensionString dest)
{
    NSError* error = nil;
    NSString* source = [NSString stringWithUTF8String:src.c_str()];
    NSString* destination = [NSString stringWithUTF8String:dest.c_str()];
    
    if ( [[NSFileManager defaultManager] isReadableFileAtPath:source] ) {
        if ( [[NSFileManager defaultManager] isReadableFileAtPath:destination] )
            [[NSFileManager defaultManager] removeItemAtPath:destination error:&error];
        
        [[NSFileManager defaultManager] copyItemAtPath:source toPath:destination error:&error];

        return ConvertNSErrorCode(error, false);
    }
    return ERR_NOT_FOUND;
}


void NSArrayToCefList(NSArray* array, CefRefPtr<CefListValue>& list)
{
    for (NSUInteger i = 0; i < [array count]; i++) {
        list->SetString(i, [[[array objectAtIndex:i] precomposedStringWithCanonicalMapping] UTF8String]);
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
        case NSFileWriteFileExistsError:
            return ERR_FILE_EXISTS;
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
  browser->GetHost()->CloseBrowser(true);
  [window close];
}

void BringBrowserWindowToFront(CefRefPtr<CefBrowser> browser)
{
  NSWindow* window = [browser->GetHost()->GetWindowHandle() window];
  [window makeKeyAndOrderFront:nil];
}

@implementation ChromeWindowsTerminatedObserver

- (void) appTerminated:(NSNotification *)note
{
    // Not Chrome? Not interested.
    if ( ![[[note userInfo] objectForKey:@"NSApplicationBundleIdentifier"] isEqualToString:appId] ) {
        return;
    }

    // Not LiveBrowser instance? Not interested.
    if ( ![[[note userInfo] objectForKey:@"NSApplicationProcessIdentifier"] isEqualToNumber:[NSNumber numberWithInt:LiveBrowserMgrMac::GetInstance()->GetLiveBrowserPid()]] ) {
        return;
    }

    LiveBrowserMgrMac::GetInstance()->CheckForChromeRunning();
}

- (void) timeoutTimer:(NSTimer*)timer
{
    LiveBrowserMgrMac::GetInstance()->CheckForChromeRunningTimeout();
}

@end

int32 ShowFolderInOSWindow(ExtensionString pathname)
{
    NSString *filepath = [NSString stringWithUTF8String:pathname.c_str()];
    BOOL isDirectory;
    
    if (![[NSFileManager defaultManager] fileExistsAtPath:filepath isDirectory:&isDirectory]) {
        return ERR_NOT_FOUND;
    }

    if (isDirectory) {
        [[NSWorkspace sharedWorkspace] openFile:filepath];
    } else {
        [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:[NSArray arrayWithObject: [NSURL fileURLWithPath: filepath]]];
    }
    return NO_ERROR;
}

int32 GetPendingFilesToOpen(ExtensionString& files)
{
    if (pendingOpenFiles) {
        NSUInteger count = [pendingOpenFiles count];
        files = "[";
        for (NSUInteger i = 0; i < count; i++) {
          NSString* filename = [pendingOpenFiles objectAtIndex:i];
      
          files += ("\"" + std::string([filename UTF8String]) + "\"");
          if (i < count - 1)
            files += ",";
        }
        files += "]";
        [pendingOpenFiles release];
        pendingOpenFiles = NULL;
    } else {
        files = "[]";
    }
    return NO_ERROR;
}

int32 GetMenuPosition(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId, ExtensionString& parentId, int& index)
{
    index = -1;
    parentId = ExtensionString();
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(commandId);
    
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }
    
    NSMenuItem* item = (NSMenuItem*)NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    NSMenu* parentMenu = NULL;
    if (item == NULL) {
        parentMenu = [NSApp mainMenu];
    } else {
        parentMenu = [item menu];
        parentId = NativeMenuModel::getInstance(getMenuParent(browser)).getParentId(tag);       
    }

    index = [parentMenu indexOfItemWithTag:tag];
    
    return NO_ERROR;
}

// Return index where menu or menu item should be placed.
// -1 indicates append.
int32 getNewMenuPosition(CefRefPtr<CefBrowser> browser, NSMenu* menu, const ExtensionString& position, const ExtensionString& relativeId, int32& positionIdx)
{
    NativeMenuModel model = NativeMenuModel::getInstance(getMenuParent(browser));
    ExtensionString pos = position;
    ExtensionString relId = relativeId;
    
    NSInteger errCode = NO_ERROR;
    if (position.size() == 0) {
        positionIdx = -1;
    } else if ((pos == "firstInSection" || pos == "lastInSection") && relId.size() > 0) {
        int32 startTag = model.getTag(relId);
        NSMenuItem* item = (NSMenuItem*)model.getOsItem(startTag);
        NSMenu* parentMenu = [item menu];
        NSInteger startIndex = [parentMenu indexOfItemWithTag:startTag];
        
        if (menu != parentMenu) {
            // Section is in a different menu.
            positionIdx = -1;
            return ERR_NOT_FOUND;
        }
        
        if (pos == "firstInSection") {
            // Move backwards until reaching the beginning of the menu or a separator
            while (startIndex >= 0) {
                if ([[parentMenu itemAtIndex:startIndex] isSeparatorItem]) {
                    break;
                }
                startIndex--;
            }
            if (startIndex < 0) {
                positionIdx = 0;
            } else {
                startIndex++;
                pos = "before";
            }
        } else { // "lastInSection"
            NSInteger numItems = [parentMenu numberOfItems];
            
            // Move forwards until reaching the end of the menu or a separator
            while (startIndex < numItems) {
                if ([[parentMenu itemAtIndex:startIndex] isSeparatorItem]) {
                    break;
                }
                startIndex++;
            }
            if (startIndex == numItems) {
                positionIdx = -1;
            } else {
                startIndex--;
                pos = "after";
            }
        }
        
        if (pos == "before" || pos == "after") {
            relId = model.getCommandId([[parentMenu itemAtIndex:startIndex] tag]);
        }
    }
        
    if ((pos == "before" || pos == "after") && relId.size() > 0) {
        ExtensionString parentId; 
        errCode = GetMenuPosition(browser, relId, parentId, positionIdx);

        if (menu && menu != [(NSMenuItem*)model.getOsItem(model.getTag(parentId)) submenu]) {
            errCode = ERR_NOT_FOUND;
        }
        
        // If we don't find the relative ID, return an error
        // and set positionIdx to -1. The item will be appended and an error will be shown.
        if (errCode == ERR_NOT_FOUND) {
            positionIdx = -1;
        }
        if (positionIdx >= 0 && pos == "after") {
            positionIdx += 1;
        }
    } else if (pos == "first") {
        positionIdx = 0;
    }

    return errCode;
}

int32 AddMenu(CefRefPtr<CefBrowser> browser, ExtensionString itemTitle, ExtensionString command, ExtensionString position, ExtensionString relativeId) {

    NSString* itemTitleStr = [[[NSString alloc] initWithUTF8String:itemTitle.c_str()] autorelease];
    NSMenuItem *testItem = nil;
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(command);
    if (tag == kTagNotFound) {
        tag = NativeMenuModel::getInstance(getMenuParent(browser)).getOrCreateTag(command, ExtensionString());
    } else {
        // menu already there
        return NO_ERROR;
    }
    NSInteger menuIdx = [[NSApp mainMenu] indexOfItemWithTag:tag];
    if (menuIdx >= 0) {
        // if we didn't find the tag, we shouldn't already have an item with this tag
        return ERR_UNKNOWN;
    } else {
        testItem = [[[NSMenuItem alloc] initWithTitle:itemTitleStr action:nil keyEquivalent:@""] autorelease];
        [testItem setTag:tag];
        NativeMenuModel::getInstance(getMenuParent(browser)).setOsItem(tag, (void*)testItem);
    }
    NSMenu *subMenu = [testItem submenu];
    if (subMenu == nil) {
        subMenu = [[[NSMenu alloc] initWithTitle:itemTitleStr] autorelease];
        [testItem setSubmenu:subMenu];
        [subMenu setDelegate:[[NSApp mainMenu] delegate]];
    }
   
    // Positioning hack. If position and relativeId are both "", put the menu
    // before the window menu *except* if it is the Help menu.
    if (position.size() == 0 && relativeId.size() == 0 && command != "help-menu") {
        position = "before";
        relativeId = "window";
    }
    
    int32 positionIdx = -1;
    int32 errCode = ::getNewMenuPosition(browser, nil, position, relativeId, positionIdx);

    // Another position hack. If position is "first" we will change positionIdx to 1
    // since we can't allow user to put anything before the Mac OS default application menu.
    if (position.size() > 0 && position == "first" && positionIdx == 0) {
        positionIdx = 1;
    }
    
    if (positionIdx > -1) {
        [[NSApp mainMenu] insertItem:testItem atIndex:positionIdx];
    } else {
        [[NSApp mainMenu] addItem:testItem];
    }
    return errCode;
}

// Replace keyStroke with replaceString
bool fixupKey(ExtensionString& key, ExtensionString keyStroke, ExtensionString replaceString)
{
    size_t idx = key.find(keyStroke, 0);
    if (idx != ExtensionString::npos) {
        key = key.replace(idx, keyStroke.size(), replaceString);
        return true;
    }
    return false;
}

// Looks at modifiers and special keys in "key",
// removes then and returns an unsigned int mask
// that can be used by setKeyEquivalentModifierMask
NSUInteger processKeyString(ExtensionString& key)
{
    // Bail early if empty string is passed
    if (key == "") {
        return 0;
    }
    NSUInteger mask = 0;
    if (fixupKey(key, "Cmd-", "")) {
        mask |= NSCommandKeyMask;
    }
    if (fixupKey(key, "Ctrl-", "")) {
        mask |= NSControlKeyMask;
    }
    if (fixupKey(key, "Shift-", "")) {
        mask |= NSShiftKeyMask;
    }
    if (fixupKey(key, "Alt-", "") ||
        fixupKey(key, "Opt-", "")) {
        mask |= NSAlternateKeyMask;
    }

    unichar pageUpChar   = NSPageUpFunctionKey;
    unichar pageDownChar = NSPageDownFunctionKey;
    unichar homeChar     = NSHomeFunctionKey;
    unichar endChar      = NSEndFunctionKey;
    unichar insertChar   = NSHelpFunctionKey;
    
    //replace special keys with ones expected by keyEquivalent
    const ExtensionString pageUp    = (ExtensionString() += [[NSString stringWithCharacters: &pageUpChar length: 1] UTF8String]);
    const ExtensionString pageDown  = (ExtensionString() += [[NSString stringWithCharacters: &pageDownChar length: 1] UTF8String]);
    const ExtensionString home      = (ExtensionString() += [[NSString stringWithCharacters: &homeChar length: 1] UTF8String]);
    const ExtensionString end       = (ExtensionString() += [[NSString stringWithCharacters: &endChar length: 1] UTF8String]);
    const ExtensionString ins       = (ExtensionString() += [[NSString stringWithCharacters: &insertChar length: 1] UTF8String]);
    const ExtensionString del       = (ExtensionString() += NSDeleteCharacter);
    const ExtensionString backspace = (ExtensionString() += NSBackspaceCharacter);
    const ExtensionString tab       = (ExtensionString() += NSTabCharacter);
    const ExtensionString enter     = (ExtensionString() += NSEnterCharacter);
    
    fixupKey(key, "PageUp", pageUp);
    fixupKey(key, "PageDown", pageDown);
    fixupKey(key, "Home", home);
    fixupKey(key, "End", end);
    fixupKey(key, "Insert", ins);
    fixupKey(key, "Delete", del);
    fixupKey(key, "Backspace", backspace);
    fixupKey(key, "Space", " ");
    fixupKey(key, "Tab", tab);
    fixupKey(key, "Enter", enter);
    fixupKey(key, "Up", "↑");
    fixupKey(key, "Down", "↓");
    fixupKey(key, "Left", "←");
    fixupKey(key, "Right", "→");

    // from unicode display char to ascii hyphen
    fixupKey(key, "−", "-");

    // Check for F1 - F15 keys.
    if (key.find("F") != ExtensionString::npos) {
        for (int i = NSF15FunctionKey; i >= NSF1FunctionKey; i--) {
            NSString *funcKey = [NSString stringWithFormat:@"F%d", i - NSF1FunctionKey + 1];
            ExtensionString fKey = [funcKey UTF8String];
            if (key.find(fKey) != ExtensionString::npos) {
                unichar ch = i;
                NSString *actualKey = [NSString stringWithCharacters: &ch length: 1];
                fixupKey(key, fKey, ExtensionString([actualKey UTF8String]));
                break;
            }
        }
    }

    return mask;
}

// displayStr is not used on the mac
int32 AddMenuItem(CefRefPtr<CefBrowser> browser, ExtensionString parentCommand, ExtensionString itemTitle, ExtensionString command, ExtensionString key, ExtensionString displayStr, ExtensionString position, ExtensionString relativeId) {
    NSString* itemTitleStr = [[[NSString alloc] initWithUTF8String:itemTitle.c_str()] autorelease];
    NSMenuItem *testItem = nil;
    int32 parentTag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(parentCommand);
    bool isSeparator = (itemTitle == "---");
    if (parentTag == kTagNotFound) {
        return NO_ERROR;
    }
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(command);
    if (tag == kTagNotFound) {
        tag = NativeMenuModel::getInstance(getMenuParent(browser)).getOrCreateTag(command, parentCommand);
    } else {
        return NO_ERROR;
    }

    NSInteger menuIdx;
    testItem = (NSMenuItem*)NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(parentTag);
    
    if (testItem != nil) {
        NSMenu* subMenu = nil;
        if (![testItem hasSubmenu]) {
            subMenu = [[[NSMenu alloc] initWithTitle:itemTitleStr] autorelease];
            [testItem setSubmenu:subMenu];
        }
        subMenu = [testItem submenu];
        if (subMenu != nil) {
            if (isSeparator) {
                menuIdx = -1;
            }
            else {
                menuIdx = [subMenu indexOfItemWithTag:tag];
            }
            
            if (menuIdx < 0) {
                NSMenuItem* newItem = nil;
                if (isSeparator) {
                    newItem = [NSMenuItem separatorItem];
                }
                else {
                    NSUInteger mask = processKeyString(key);
                    NSString* keyStr = [[[NSString alloc] initWithUTF8String:key.c_str()] autorelease];
                    keyStr = [keyStr lowercaseString];
                    newItem = [[NSMenuItem alloc] autorelease];
                    [newItem setTitle:itemTitleStr];
                    [newItem setAction:NSSelectorFromString(@"handleMenuAction:")];
                    [newItem setKeyEquivalent:keyStr];
                    [newItem setKeyEquivalentModifierMask:mask];
                    [newItem setTag:tag];
                }
                NativeMenuModel::getInstance(getMenuParent(browser)).setOsItem(tag, (void*)newItem);
                
                int32 positionIdx = -1;
                int32 errCode = ::getNewMenuPosition(browser, subMenu, position, relativeId, positionIdx);
                if (positionIdx > -1) {
                    [subMenu insertItem:newItem atIndex:positionIdx];
                } else {
                    [subMenu addItem:newItem];
                }
                return errCode;
            }
        }
    }

    return NO_ERROR;
}

int32 GetMenuItemState(CefRefPtr<CefBrowser> browser, ExtensionString commandId, bool& enabled, bool &checked, int& index)
{
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(commandId);
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }
    NSMenuItem* item = (NSMenuItem*)NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    if (item == NULL) {
        return ERR_NOT_FOUND;
    }
    if ([item respondsToSelector:@selector(menu)]) {
        NSWindow* mainWindow = [NSApp mainWindow];
        //menu item's enabled status is dependent on the selector's return value.
        //[item enabled] will only be correct if we use manual menu enablement.
        enabled = [(NSObject*)[mainWindow delegate] performSelector:@selector(validateMenuItem:) withObject:item];
        checked = ([item state] == NSOnState);
        index = [[item menu] indexOfItemWithTag:tag];
    }
    return NO_ERROR;
}

int32 SetMenuItemState(CefRefPtr<CefBrowser> browser, ExtensionString command, bool& enabled, bool& checked)
{
    return NO_ERROR;
}

int32 SetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString command, ExtensionString itemTitle) {
    NSString* itemTitleStr = [[[NSString alloc] initWithUTF8String:itemTitle.c_str()] autorelease];
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(command);
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }

    NSMenuItem* menuItem = (NSMenuItem*)NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    if (menuItem == NULL) {
        return ERR_NOT_FOUND;
    }
    
    if ([menuItem submenu]) {
        [[menuItem submenu] setTitle:itemTitleStr];
    } else {
        [menuItem setTitle:itemTitleStr];
    }

    return NO_ERROR;
}

OSStatus _RunToolWithAdminPrivileges( AuthorizationRef authorizationRef, const std::string& tool, const std::vector<std::string> &args)
{
    OSStatus status = 0;
    if(!authorizationRef || tool.length() ==  0 || args.size() == 0)
        return errAuthorizationInvalidSet;
    
    FILE *pipe = NULL;

    // convert to std::vector<char *> array.
    std::vector<char*> argv(1 + args.size(), NULL);

    for (size_t i = 0; i < args.size(); ++i)
        argv[i] = const_cast<char*>(args[i].c_str());
    
    // This is a deprecated API. Apple recommends a dedicated helper to
    // fix this issue. This ideally should be replaced by SMBless API.
    status = AuthorizationExecuteWithPrivileges(authorizationRef, tool.c_str(),
                                                kAuthorizationFlagDefaults, &argv[0], &pipe);
    
    if(pipe){
        fclose(pipe);
    }
    
    return status;
    
}

int32 InstallCommandLineTools()
{
    // Create authorization reference
    OSStatus authStatus  = 0;
    OSStatus toolStatus  = 0;
    int32    errorCode   = NO_ERROR;


    std::string destFile   = "/usr/local/bin/brackets";
    std::string destFolder = "/usr/local/bin";

    std::string rmTool     = "/bin/rm";
    std::string rmArgs     = "-f";

    std::string mkDirTool  = "/bin/mkdir";
    std::string mkDirArgs  = "-p";

    std::string lnTool     = "/bin/ln";
    std::string lnArgs     = "-s";


    AuthorizationRef authorizationRef = NULL;
    
    try {
        
        // Determine the location of the launch script.
        // We have this file, brackets.sh, present inside resource folder.
        

        NSString* sourcePath;
        
        sourcePath = [[NSBundle mainBundle] pathForResource: @"brackets" ofType: @"sh"];
        if(!sourcePath) {
            // If we have not found this in the bundle try the hard path.
            NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
            sourcePath = [bundlePath stringByAppendingString:@"/Contents/Resources/brackets.sh"];
        }
        
        if(!sourcePath)
            return ERR_FILE_NOT_FOUND;
        
        std::string sourceFile = [sourcePath UTF8String];
        
        // AuthorizationCreate and pass NULL as the initial
        // AuthorizationRights set so that the AuthorizationRef gets created
        // successfully.
        // http://developer.apple.com/qa/qa2001/qa1172.html
        
        authStatus = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment,
                                     kAuthorizationFlagDefaults, &authorizationRef);
        
        if(authStatus == errAuthorizationSuccess) {
            

            std::vector<std::string> args;

            // Now execute all these steps one by one
            //  1. Remove existing symlink
            //  2. Check for existence of the directory and create one if required.
            //  3  Create symlink at /usr/local/bin.
            
            // *** Removing the existing symlink ***
            
            args.push_back(rmArgs);
            args.push_back(destFile);
            
            toolStatus = _RunToolWithAdminPrivileges(authorizationRef, rmTool, args);
            
            if( toolStatus == errAuthorizationSuccess) {
                
                // *** Check and create if the directory is not present ***
                
                args.clear();
                
                args.push_back(mkDirArgs);
                args.push_back(destFolder);
                
                toolStatus = _RunToolWithAdminPrivileges(authorizationRef, mkDirTool, args);
                
                if( toolStatus == errAuthorizationSuccess) {
                    
                    // *** Go ahead and create the symlink now ***
                    
                    args.clear();
                    
                    args.push_back(lnArgs);
                    args.push_back(sourceFile);
                    args.push_back(destFile);
                    
                    toolStatus = _RunToolWithAdminPrivileges(authorizationRef, lnTool, args);

                    if( toolStatus != errAuthorizationSuccess)
                        errorCode = ERR_CL_TOOLS_SYMLINKFAILED;
                    
                }
                else {
                    errorCode = ERR_CL_TOOLS_MKDIRFAILED;
                }
            }
            else{
                if(toolStatus == errAuthorizationCanceled)
                    errorCode = ERR_CL_TOOLS_CANCELLED;
                else
                    errorCode = ERR_CL_TOOLS_RMFAILED;
            }

        }
    
    }
    catch (...) {
        // This is empty as the below statements will take care of
        // releasing authorizationRef.
        errorCode = ERR_CL_TOOLS_SERVFAILED;
    }
    
    if(authorizationRef) {
        AuthorizationFree(authorizationRef, kAuthorizationFlagDestroyRights);
    }

    return errorCode;

}

int32 GetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString& title)
{
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(commandId);
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }
    NSMenuItem* item = (NSMenuItem*)NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    if (item == NULL) {
        return ERR_NOT_FOUND;
    }
    
    if ([item submenu]) {
        title = [[[item submenu] title] UTF8String];
    } else {
        title = [[item title] UTF8String];
    }
    
    return NO_ERROR;
}

// The displayStr param is ignored on the mac.
int32 SetMenuItemShortcut(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString shortcut, ExtensionString displayStr)
{
    NativeMenuModel model = NativeMenuModel::getInstance(getMenuParent(browser));

    int32 tag = model.getTag(commandId);
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }
    NSMenuItem* item = (NSMenuItem*)model.getOsItem(tag);
    if (item == NULL) {
        return ERR_NOT_FOUND;
    }
    
    NSUInteger mask = processKeyString(shortcut);
    NSString* keyStr = [[[NSString alloc] initWithUTF8String:shortcut.c_str()] autorelease];
    keyStr = [keyStr lowercaseString];
    [item setKeyEquivalent:keyStr];
    [item setKeyEquivalentModifierMask:mask];
    
    return NO_ERROR;
}

//Remove menu item associated with commandId
int32 RemoveMenu(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId)
{
    //works for menu and menu item
    return RemoveMenuItem(browser, commandId);
}

//Remove menu item associated with commandId
int32 RemoveMenuItem(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId)
{
    int tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(commandId);
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }
    NSMenuItem* item = (NSMenuItem*)NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    if (item == NULL) {
        return ERR_NOT_FOUND;
    }
    
    NSMenu* parentMenu = NULL;
    if ([item respondsToSelector:@selector(menu)]) {
        parentMenu = [item menu];
        if (parentMenu == NULL) {
            return ERR_NOT_FOUND;
        }
        [parentMenu removeItem:item];
        NativeMenuModel::getInstance(getMenuParent(browser)).removeMenuItem(commandId);
    } else {
        return ERR_NOT_FOUND;
    }
    
    return NO_ERROR;
}

void DragWindow(CefRefPtr<CefBrowser> browser)
{
    NSWindow* win = [browser->GetHost()->GetWindowHandle() window];
    NSPoint origin = [win frame].origin;
    NSPoint current = [NSEvent mouseLocation];
    NSPoint offset;
    origin.x -= current.x;
    origin.y -= current.y;

    while (YES) {
        NSEvent* event = [win nextEventMatchingMask:(NSLeftMouseDraggedMask | NSLeftMouseUpMask)];
        if ([event type] == NSLeftMouseUp) break;
        current = [win convertBaseToScreen:[event locationInWindow]];
        offset = origin;
        offset.x += current.x;
        offset.y += current.y;
        [win setFrameOrigin:offset];
        [win displayIfNeeded];
    }
}

int32 GetArgvFromProcessID(int pid, NSString **argv);
NSRunningApplication* GetLiveBrowserApp(NSString *bundleId, int debugPort)
{

    NSArray* appList = [NSRunningApplication runningApplicationsWithBundleIdentifier: bundleId];

    // Search list of running apps with bundleId + debug port
    for (NSRunningApplication* currApp in appList) {

        int PID = [currApp processIdentifier];
        NSString* args = nil;

        // Check for process arguments
        if (GetArgvFromProcessID(PID, &args) != NO_ERROR) {
            continue;
        }

        // Check debug port (e.g. --remote-debugging-port=9222)
        if ([args rangeOfString:debugPortCommandlineArguments].location != NSNotFound) {
            return currApp;
        }
    }
    return nil;
}

// Extracted & Modified from https://gist.github.com/nonowarn/770696
int32 GetArgvFromProcessID(int pid, NSString **argv)
{
    int    mib[3], argmax, nargs, c = 0;
    size_t    size;
    char    *procargs, *sp, *np, *cp;
    int show_args = 1;

    mib[0] = CTL_KERN;
    mib[1] = KERN_ARGMAX;

    size = sizeof(argmax);
    if (sysctl(mib, 2, &argmax, &size, NULL, 0) == -1) {
        goto ERROR_A;
    }

    /* Allocate space for the arguments. */
    procargs = (char *)malloc(argmax);
    if (procargs == NULL) {
        goto ERROR_A;
    }


    /*
     * Make a sysctl() call to get the raw argument space of the process.
     * The layout is documented in start.s, which is part of the Csu
     * project.  In summary, it looks like:
     *
     * /---------------\ 0x00000000
     * :               :
     * :               :
     * |---------------|
     * | argc          |
     * |---------------|
     * | arg[0]        |
     * |---------------|
     * :               :
     * :               :
     * |---------------|
     * | arg[argc - 1] |
     * |---------------|
     * | 0             |
     * |---------------|
     * | env[0]        |
     * |---------------|
     * :               :
     * :               :
     * |---------------|
     * | env[n]        |
     * |---------------|
     * | 0             |
     * |---------------| <-- Beginning of data returned by sysctl() is here.
     * | argc          |
     * |---------------|
     * | exec_path     |
     * |:::::::::::::::|
     * |               |
     * | String area.  |
     * |               |
     * |---------------| <-- Top of stack.
     * :               :
     * :               :
     * \---------------/ 0xffffffff
     */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROCARGS2;
    mib[2] = pid;

    size = (size_t)argmax;
    if (sysctl(mib, 3, procargs, &size, NULL, 0) == -1) {
        goto ERROR_B;
    }

    memcpy(&nargs, procargs, sizeof(nargs));
    cp = procargs + sizeof(nargs);

    /* Skip the saved exec_path. */
    for (; cp < &procargs[size]; cp++) {
        if (*cp == '\0') {
            /* End of exec_path reached. */
            break;
        }
    }
    if (cp == &procargs[size]) {
        goto ERROR_B;
    }

    /* Skip trailing '\0' characters. */
    for (; cp < &procargs[size]; cp++) {
        if (*cp != '\0') {
            /* Beginning of first argument reached. */
            break;
        }
    }
    if (cp == &procargs[size]) {
        goto ERROR_B;
    }
    /* Save where the argv[0] string starts. */
    sp = cp;

    /*
     * Iterate through the '\0'-terminated strings and convert '\0' to ' '
     * until a string is found that has a '=' character in it (or there are
     * no more strings in procargs).  There is no way to deterministically
     * know where the command arguments end and the environment strings
     * start, which is why the '=' character is searched for as a heuristic.
     */
    for (np = NULL; c < nargs && cp < &procargs[size]; cp++) {
        if (*cp == '\0') {
            c++;
            if (np != NULL) {
                /* Convert previous '\0'. */
                *np = ' ';
            } else {
                /* *argv0len = cp - sp; */
            }
            /* Note location of current '\0'. */
            np = cp;

            if (!show_args) {
                /*
                 * Don't convert '\0' characters to ' '.
                 * However, we needed to know that the
                 * command name was terminated, which we
                 * now know.
                 */
                break;
            }
        }
    }

    /*
     * sp points to the beginning of the arguments/environment string, and
     * np should point to the '\0' terminator for the string.
     */
    if (np == NULL || np == sp) {
        /* Empty or unterminated string. */
        goto ERROR_B;
    }

    *argv = [NSString stringWithCString:sp encoding:NSUTF8StringEncoding];

    /* Clean up. */
    free(procargs);

    return NO_ERROR;

ERROR_B:
    free(procargs);
ERROR_A:
    return ERR_UNKNOWN;
}


// The below code is from https://oroboro.com/unique-machine-fingerprint/ and modified
// Original Author: Rafael

//---------------------------------get MAC addresses ---------------------------------
// we just need this for purposes of unique machine id. So any one or two
// mac's is fine.

u16 HashMacAddress( u8* mac )
{
    u16 hash = 0;
    
    for ( u32 i = 0; i < 6; i++ )
    {
        hash += ( mac[i] << (( i & 1 ) * 8 ));
    }
    return hash;
}

void GetMacHash( u16& mac1, u16& mac2 )
{
    mac1 = 0;
    mac2 = 0;
    
    
    struct ifaddrs* ifaphead;
    if ( getifaddrs( &ifaphead ) != 0 )
        return;
    
    // iterate over the net interfaces
    bool foundMac1 = false;
    struct ifaddrs* ifap;
    for ( ifap = ifaphead; ifap; ifap = ifap->ifa_next )
    {
        struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifap->ifa_addr;
        if ( sdl && ( sdl->sdl_family == AF_LINK ) && ( sdl->sdl_type == IFT_ETHER ))
        {
            if ( !foundMac1 )
            {
                foundMac1 = true;
                mac1 = HashMacAddress( (u8*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );
            } else {
                mac2 = HashMacAddress( (u8*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );
                break;
            }
        }
    }
    
    freeifaddrs( ifaphead );
    
    
    // sort the mac addresses. We don't want to invalidate
    // both macs if they just change order.
    if ( mac1 > mac2 )
    {
        u16 tmp = mac2;
        mac2 = mac1;
        mac1 = tmp;
    }
}

const char* GetMachineName()
{
    static struct utsname u;

    if ( uname( &u ) < 0 )
    {
        assert(0);
        return "unknown";
    }

    return u.nodename;
}

u16 GetVolumeHash()
{
    // we don't have a 'volume serial number' like on windows.
    // Lets hash the system name instead.
    u8* sysname = (u8*)GetMachineName();
    u16 hash = 0;
    
    for ( u32 i = 0; sysname[i]; i++ )
        hash += ( sysname[i] << (( i & 1 ) * 8 ));
    
    return hash;
}

u16 GetCPUHash()
{
    const NXArchInfo* info = NXGetLocalArchInfo();
    u16 val = 0;
    val += (u16)info->cputype;
    val += (u16)info->cpusubtype;
    return val;
}

u16 mask[5] = { 0x4e25, 0xf4a1, 0x5437, 0xab41, 0x0000 };

static void Smear(u16* id)
{
	for (u32 i = 0; i < 5; i++)
		for (u32 j = i; j < 5; j++)
			if (i != j)
				id[i] ^= id[j];

	for (u32 i = 0; i < 5; i++)
		id[i] ^= mask[i];
}

static u16* ComputeSystemUniqueID()
{
	static u16 id[5] = {0};
	static bool computed = false;

	if (computed) return id;

	// produce a number that uniquely identifies this system.
	id[0] = GetCPUHash();
	id[1] = GetVolumeHash();
	GetMacHash(id[2], id[3]);

	// fifth block is some check digits
	id[4] = 0;
	for (u32 i = 0; i < 4; i++)
		id[4] += id[i];

	Smear(id);

	computed = true;
	return id;
}

std::string GetSystemUniqueID()
{
	// get the name of the computer
	std::string buf;

	u16* id = ComputeSystemUniqueID();
	for (u32 i = 0; i < 5; i++)
	{
		char num[16];
		snprintf(num, 16, "%x", id[i]);
		if (i > 0) {
			buf = buf + "-";
		}
		switch (strlen(num))
		{
		case 1: buf = buf + "000"; break;
		case 2: buf = buf + "00";  break;
		case 3: buf = buf + "0";   break;
		}
		buf = buf + num;
	}
	
	return buf;
}
