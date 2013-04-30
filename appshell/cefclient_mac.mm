// Copyright (c) 2010 The Chromium Embedded Framework Authors.
// Portions copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include <sstream>
#include "cefclient.h"
#include "include/cef_app.h"
#import "include/cef_application_mac.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "client_handler.h"
#include "resource_util.h"
#include "string_util.h"
#include "config.h"
#include "appshell_extensions.h"
#include "command_callbacks.h"
#include "client_switches.h"
#include "native_menu_model.h"
#include "appshell_node_process.h"

// Application startup time
CFTimeInterval g_appStartupTime;

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;
static bool g_isTerminating = false;

char szWorkingDir[512];   // The current working directory

NSURL* startupUrl = nil;

#ifdef SHOW_TOOLBAR_UI
// Sizes for URL bar layout
#define BUTTON_HEIGHT 22
#define BUTTON_WIDTH 72
#define BUTTON_MARGIN 8
#define URLBAR_HEIGHT  32
#endif // SHOW_TOOLBAR_UI

// Content area size for newly created windows.
const int kWindowWidth = 1000;
const int kWindowHeight = 700;

// Memory AutoRelease pool.
static NSAutoreleasePool* g_autopool = nil;

// Files passed to the app at startup
static NSMutableArray* pendingOpenFiles;
extern ExtensionString gPendingFilesToOpen;

// Provide the CefAppProtocol implementation required by CEF.
@interface ClientApplication : NSApplication<CefAppProtocol> {
@private
  BOOL handlingSendEvent_;
}
@end

@implementation ClientApplication
- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

- (void)sendEvent:(NSEvent*)event {
  CefScopedSendingEvent sendingEventScoper;
  [super sendEvent:event];
}
@end


// Receives notifications from controls and the browser window. Will delete
// itself when done.
@interface ClientWindowDelegate : NSObject <NSWindowDelegate> {
  BOOL isReallyClosing;
}
- (void)setIsReallyClosing;
- (IBAction)handleMenuAction:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem;
- (IBAction)showAbout:(id)sender;
- (IBAction)quit:(id)sender;
#ifdef SHOW_TOOLBAR_UI
- (IBAction)goBack:(id)sender;
- (IBAction)goForward:(id)sender;
- (IBAction)reload:(id)sender;
- (IBAction)stopLoading:(id)sender;
- (IBAction)takeURLStringValueFrom:(NSTextField *)sender;
#endif // SHOW_TOOLBAR_UI
- (void)alert:(NSString*)title withMessage:(NSString*)message;
- (void)notifyConsoleMessage:(id)object;
- (void)notifyDownloadComplete:(id)object;
- (void)notifyDownloadError:(id)object;
@end

@implementation ClientWindowDelegate

- (id) init {
  [super init];
  isReallyClosing = false;
  return self;
}

- (void)setIsReallyClosing {
  isReallyClosing = true;
}

- (IBAction)showAbout:(id)sender {
  if (g_handler.get() && g_handler->GetBrowserId())
    g_handler->SendJSCommand(g_handler->GetBrowser(), HELP_ABOUT);
}

- (IBAction)handleMenuAction:(id)sender {
    if (g_handler.get() && g_handler->GetBrowserId()) {
        NSMenuItem* senderItem = sender;
        NSUInteger tag = [senderItem tag];
        ExtensionString commandId = NativeMenuModel::getInstance(getMenuParent(g_handler->GetBrowser())).getCommandId(tag);
        CefRefPtr<CommandCallback> callback = new EditCommandCallback(g_handler->GetBrowser(), commandId);
        g_handler->SendJSCommand(g_handler->GetBrowser(), commandId, callback);
    }
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    NSInteger menuState = NSOffState;
    NSUInteger tag = [menuItem tag];
    NativeMenuModel menus = NativeMenuModel::getInstance(getMenuParent(g_handler->GetBrowser()));
    if (menus.isMenuItemChecked(tag)) {
        menuState = NSOnState;
    }
    [menuItem setState:menuState];
    return menus.isMenuItemEnabled(tag);
}

- (IBAction)quit:(id)sender {
  /*
  if (g_handler.get() && g_handler->GetBrowserId()) {
    g_handler->SendJSCommand(g_handler->GetBrowser(), FILE_QUIT);
  } else {
    [NSApp terminate:nil];
  }
  */
  g_handler->DispatchCloseToNextBrowser();
}

#ifdef SHOW_TOOLBAR_UI
- (IBAction)goBack:(id)sender {
  if (g_handler.get() && g_handler->GetBrowserId())
    g_handler->GetBrowser()->GoBack();
}

- (IBAction)goForward:(id)sender {
  if (g_handler.get() && g_handler->GetBrowserId())
    g_handler->GetBrowser()->GoForward();
}

- (IBAction)reload:(id)sender {
  if (g_handler.get() && g_handler->GetBrowserId())
    g_handler->GetBrowser()->Reload();
}

- (IBAction)stopLoading:(id)sender {
  if (g_handler.get() && g_handler->GetBrowserId())
    g_handler->GetBrowser()->StopLoad();
}

- (IBAction)takeURLStringValueFrom:(NSTextField *)sender {
  if (!g_handler.get() || !g_handler->GetBrowserId())
    return;
  
  NSString *url = [sender stringValue];
  
  // if it doesn't already have a prefix, add http. If we can't parse it,
  // just don't bother rather than making things worse.
  NSURL* tempUrl = [NSURL URLWithString:url];
  if (tempUrl && ![tempUrl scheme])
    url = [@"http://" stringByAppendingString:url];
  
  std::string urlStr = [url UTF8String];
  g_handler->GetBrowser()->GetMainFrame()->LoadURL(urlStr);
}
#endif // SHOW_TOOLBAR_UI

- (void)alert:(NSString*)title withMessage:(NSString*)message {
  NSAlert *alert = [NSAlert alertWithMessageText:title
                                   defaultButton:@"OK"
                                 alternateButton:nil
                                     otherButton:nil
                       informativeTextWithFormat:@"%@", message];
  [alert runModal];
}

- (void)notifyConsoleMessage:(id)object {
  /*
  std::stringstream ss;
  ss << "Console messages will be written to " << g_handler->GetLogFile();
  NSString* str = [NSString stringWithUTF8String:(ss.str().c_str())];
  [self alert:@"Console Messages" withMessage:str];
  */
}

- (void)notifyDownloadComplete:(id)object {
  std::stringstream ss;
  ss << "File \"" << g_handler->GetLastDownloadFile() <<
      "\" downloaded successfully.";
  NSString* str = [NSString stringWithUTF8String:(ss.str().c_str())];
  [self alert:@"File Download" withMessage:str];
}

- (void)notifyDownloadError:(id)object {
  std::stringstream ss;
  ss << "File \"" << g_handler->GetLastDownloadFile() <<
      "\" failed to download.";
  NSString* str = [NSString stringWithUTF8String:(ss.str().c_str())];
  [self alert:@"File Download" withMessage:str];
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
  if (g_handler.get() && g_handler->GetBrowserId()) {
    // Give focus to the browser window.
    g_handler->GetBrowser()->GetHost()->SetFocus(true);
  }
}

- (void)windowDidResignKey:(NSNotification *)notification {
	if (g_handler.get() && g_handler->GetBrowserId()) {
		// Remove focus from the browser window.
		g_handler->GetBrowser()->GetHost()->SetFocus(false);
	}
	[[notification object] makeFirstResponder: nil];
}

// Called when the window is about to close. Perform the self-destruction
// sequence by getting rid of the window. By returning YES, we allow the window
// to be removed from the screen.
- (BOOL)windowShouldClose:(id)window { 
  
  // This function can be called as many as THREE times for each window.
  // The first time will dispatch a FILE_CLOSE_WINDOW command and return NO. 
  // This gives the JavaScript the first crack at handling the command, which may result
  // is a "Save Changes" dialog being shown.
  // If the user chose to save changes (or ignore changes), the JavaScript calls window.close(),
  // which results in a second call to this function. This time we dispatch another FILE_CLOSE_WINDOW
  // command and return NO again.
  // The second time the command is called it returns false. In that case, the CloseWindowCommandCallback
  // will call browser->GetHost()->CloseBrowser(). However, before calling CloseBrowser(), the "isReallyClosing"
  // flag is set. The CloseBrowser() call results in a THIRD call to this function, but this time we check
  // the isReallyClosing flag and if true, return YES.

  if (!isReallyClosing && g_handler.get() && g_handler->GetBrowserId()) {
    CefRefPtr<CommandCallback> callback = new CloseWindowCommandCallback(g_handler->GetBrowser());
    
    g_handler->SendJSCommand(g_handler->GetBrowser(), FILE_CLOSE_WINDOW, callback);
    return NO;
  }
  
  // Try to make the window go away.
  [window autorelease];
  
  // Clean ourselves up after clearing the stack of anything that might have the
  // window on it.
  [(NSObject*)[window delegate] performSelectorOnMainThread:@selector(cleanup:)
                                                 withObject:window  waitUntilDone:NO];
  
  return YES;
}

// Deletes itself.
- (void)cleanup:(id)window {  
  [self release];
}

@end

#ifdef SHOW_TOOLBAR_UI

NSButton* MakeButton(NSRect* rect, NSString* title, NSView* parent) {
  NSButton* button = [[[NSButton alloc] initWithFrame:*rect] autorelease];
  [button setTitle:title];
  [button setBezelStyle:NSSmallSquareBezelStyle];
  [button setAutoresizingMask:(NSViewMaxXMargin | NSViewMinYMargin)];
  [parent addSubview:button];
  rect->origin.x += BUTTON_WIDTH;
  return button;
}
#endif // SHOW_TOOLBAR_UI

// Receives notifications from the application. Will delete itself when done.
@interface ClientAppDelegate : NSObject
- (void)createApp:(id)object;
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (BOOL)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames;
@end

@implementation ClientAppDelegate

// Create the application on the UI thread.
- (void)createApp:(id)object {
  [NSApplication sharedApplication];
  [NSBundle loadNibNamed:@"MainMenu" owner:NSApp];
  
  // Set the delegate for application events.
  [NSApp setDelegate:self];
  
  // Create the delegate for control and browser window events.
  ClientWindowDelegate* delegate = [[ClientWindowDelegate alloc] init];
  
  // Create the main application window.
  NSUInteger styleMask = (NSTitledWindowMask |
                          NSClosableWindowMask |
                          NSMiniaturizableWindowMask |
                          NSResizableWindowMask );

  // Get the available screen space
  NSRect screen_rect = [[NSScreen mainScreen] visibleFrame];
  // Start out with the content being as big as possible
  NSRect content_rect = [NSWindow contentRectForFrameRect:screen_rect styleMask:styleMask];
  
  // Determine the maximum height
  const int maxHeight = kWindowHeight
  #ifdef SHOW_TOOLBAR_UI
    + URLBAR_HEIGHT
  #endif
  ;
  // Make the content rect fit into maxHeight and kWindowWidth
  if (content_rect.size.height > maxHeight) {
    // First move the window up as much as we reduce it's height so it opens in the top left corner
    content_rect.origin.y += content_rect.size.height - maxHeight;
    content_rect.size.height = maxHeight;
  }
  if (content_rect.size.width > kWindowWidth) {
    content_rect.size.width = kWindowWidth;
  }

  // Initialize the window with the adjusted default size
  NSWindow* mainWnd = [[UnderlayOpenGLHostingWindow alloc]
                       initWithContentRect:content_rect
                       styleMask:styleMask
                       backing:NSBackingStoreBuffered
                       defer:NO];

  // "Preclude the window controller from changing a window’s position from the
  // one saved in the defaults system" (NSWindow Class Reference)
  [[mainWnd windowController] setShouldCascadeWindows: NO];
  
  // Set the "autosave" name for the window. If there is a previously stored
  // size for the window, it will be loaded here and used to resize the window.
  // It appears that if the stored size is too big for the screen,
  // it is automatically adjusted to fit.
  [mainWnd setFrameAutosaveName:APP_NAME @"MainWindow"];
  
  // Get the actual content size of the window since setFrameAutosaveName could
  // result in the window size changing.
  content_rect = [mainWnd contentRectForFrameRect:[mainWnd frame]];

  // Configure the rest of the window
  [mainWnd setTitle:WINDOW_TITLE];
  [mainWnd setDelegate:delegate];
  [mainWnd setCollectionBehavior: (1 << 7) /* NSWindowCollectionBehaviorFullScreenPrimary */];

  // Rely on the window delegate to clean us up rather than immediately
  // releasing when the window gets closed. We use the delegate to do
  // everything from the autorelease pool so the window isn't on the stack
  // during cleanup (ie, a window close from javascript).
  [mainWnd setReleasedWhenClosed:NO];

  NSView* contentView = [mainWnd contentView];

#ifdef SHOW_TOOLBAR_UI
  // Create the buttons.
  NSRect button_rect = [contentView bounds];
  button_rect.origin.y = content_rect.size.height - URLBAR_HEIGHT +
      (URLBAR_HEIGHT - BUTTON_HEIGHT) / 2;
  button_rect.size.height = BUTTON_HEIGHT;
  button_rect.origin.x += BUTTON_MARGIN;
  button_rect.size.width = BUTTON_WIDTH;

  NSButton* button = MakeButton(&button_rect, @"Back", contentView);
  [button setTarget:delegate];
  [button setAction:@selector(goBack:)];

  button = MakeButton(&button_rect, @"Forward", contentView);
  [button setTarget:delegate];
  [button setAction:@selector(goForward:)];

  button = MakeButton(&button_rect, @"Reload", contentView);
  [button setTarget:delegate];
  [button setAction:@selector(reload:)];

  button = MakeButton(&button_rect, @"Stop", contentView);
  [button setTarget:delegate];
  [button setAction:@selector(stopLoading:)];

  // Create the URL text field.
  button_rect.origin.x += BUTTON_MARGIN;
  button_rect.size.width = [contentView bounds].size.width -
      button_rect.origin.x - BUTTON_MARGIN;
  NSTextField* editWnd = [[NSTextField alloc] initWithFrame:button_rect];
  [contentView addSubview:editWnd];
  [editWnd setAutoresizingMask:(NSViewWidthSizable | NSViewMinYMargin)];
  [editWnd setTarget:delegate];
  [editWnd setAction:@selector(takeURLStringValueFrom:)];
  [[editWnd cell] setWraps:NO];
  [[editWnd cell] setScrollable:YES];
#endif // SHOW_TOOLBAR_UI
	
  // Create the handler.
  g_handler = new ClientHandler();
  g_handler->SetMainHwnd(contentView);
	
#ifdef SHOW_TOOLBAR_UI
  g_handler->SetEditHwnd(editWnd);
#endif // SHOW_TOOLBAR_UI
	
  // Create the browser view.
  CefWindowInfo window_info;
  CefBrowserSettings settings;

  // Populate the settings based on command line arguments.
  AppGetBrowserSettings(settings);

  settings.web_security_disabled = true;
  
  window_info.SetAsChild(contentView, 0, 0, content_rect.size.width, content_rect.size.height);
  
  NSString* str = [[startupUrl absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
  CefBrowserHost::CreateBrowserSync(window_info, g_handler.get(),
                                [str UTF8String], settings);
 
  if (pendingOpenFiles) {
    NSUInteger count = [pendingOpenFiles count];
    gPendingFilesToOpen = "[";
    for (NSUInteger i = 0; i < count; i++) {
      NSString* filename = [pendingOpenFiles objectAtIndex:i];
          
      gPendingFilesToOpen += ("\"" + std::string([filename UTF8String]) + "\"");
      if (i < count - 1)
        gPendingFilesToOpen += ",";
    }
    gPendingFilesToOpen += "]";
  } else {
    gPendingFilesToOpen = "[]";
  }
  
  // Show the window.
  [mainWnd display];
  [mainWnd makeKeyAndOrderFront: nil];
}

// Sent by the default notification center immediately before the application
// terminates.
- (void)applicationWillTerminate:(NSNotification *)aNotification {

  OnBeforeShutdown();
  
  // Shut down CEF.
  g_handler = NULL;
  CefShutdown();

  [self release];

  // Release the AutoRelease pool.
  [g_autopool release];
}


-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)theApplication {
    if (!g_isTerminating && g_handler.get() && !g_handler->AppIsQuitting() && g_handler->HasWindows() && [NSApp keyWindow]) {
        g_handler->DispatchCloseToNextBrowser();
        return NSTerminateCancel;
    }
    g_isTerminating = true;
    return NSTerminateNow;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename {
  if (g_handler) {
    CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow([NSApp keyWindow]);
    g_handler->SendOpenFileCommand(browser, CefString([filename UTF8String]));
  } else {
    // App is just starting up. Save the filename so we can open it later.
    if (!pendingOpenFiles) {
      pendingOpenFiles = [[NSMutableArray alloc] init];
      [pendingOpenFiles addObject:filename];
    }
  }
  return YES;
}

- (BOOL)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames {
  if (g_handler) {
    CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow([NSApp keyWindow]);
    for (NSUInteger i = 0; i < [filenames count]; i++) {
      g_handler->SendOpenFileCommand(browser, CefString([[filenames objectAtIndex:i] UTF8String]));
    }
  } else {
    // App is just starting up. Save the filenames so we can open them later.
    pendingOpenFiles = [[NSMutableArray alloc] init];
    for (NSUInteger i = 0; i < [filenames count]; i++) {
      [pendingOpenFiles addObject:[filenames objectAtIndex:i]];
    }
  }
  return YES;
}
@end


int main(int argc, char* argv[]) {
  // Initialize the AutoRelease pool.
  g_autopool = [[NSAutoreleasePool alloc] init];

  pendingOpenFiles = nil;
  
  CefMainArgs main_args(argc, argv);
 
  // Delete Special Characters Palette from Edit menu.
  [[NSUserDefaults standardUserDefaults]
    setBool:YES forKey:@"NSDisabledCharacterPaletteMenuItem"];
    
  g_appStartupTime = CFAbsoluteTimeGetCurrent();

  // Start the node server process
  startNodeProcess();
    
  CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app.get());
  if (exit_code >= 0)
    return exit_code;

  // Retrieve the current working directory.
  getcwd(szWorkingDir, sizeof(szWorkingDir));

  // Initialize the ClientApplication instance.
  [ClientApplication sharedApplication];
  NSObject* delegate = [[ClientAppDelegate alloc] init];
  [NSApp setDelegate:delegate];
  
  // Parse command line arguments.
  AppInitCommandLine(argc, argv);

  CefSettings settings;

  // Populate the settings based on command line arguments.
  AppGetSettings(settings, app);

  // Check command
  if (CefString(&settings.cache_path).length() == 0) {
	  CefString(&settings.cache_path) = AppGetCachePath();
  }

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get());

  // Load the startup path from prefs
  CGEventRef event = CGEventCreate(NULL);
  CGEventFlags modifiers = CGEventGetFlags(event);
  CFRelease(event);
  
  CefRefPtr<CefCommandLine> cmdLine = AppGetCommandLine();
  if (cmdLine->HasSwitch(cefclient::kStartupPath)) {
    CefString cmdLineStartupURL = cmdLine->GetSwitchValue(cefclient::kStartupPath);
    std::string startupURLStr(cmdLineStartupURL);
    NSString* str = [NSString stringWithUTF8String:startupURLStr.c_str()];
    startupUrl = [NSURL fileURLWithPath:[str stringByExpandingTildeInPath]];
  }
  else {
    // If the shift key is not pressed, look for index.html bundled in the app package
    if ((modifiers & kCGEventFlagMaskShift) != kCGEventFlagMaskShift) {
      NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
      
      // First, look in our app package for /Contents/dev/src/index.html
      NSString* devFile = [bundlePath stringByAppendingString:@"/Contents/dev/src/index.html"];
      
      if ([[NSFileManager defaultManager] fileExistsAtPath:devFile]) {
        startupUrl = [NSURL fileURLWithPath:devFile];
      }
      
      if (startupUrl == nil) {
        // If the dev file wasn't found, look for /Contents/www/index.html
        NSString* indexFile = [bundlePath stringByAppendingString:@"/Contents/www/index.html"];
        if ([[NSFileManager defaultManager] fileExistsAtPath:indexFile]) {
          startupUrl = [NSURL fileURLWithPath:indexFile];
        }
      }
    }
  }
  
  if (startupUrl == nil) {
    // If we got here, either the startup file couldn't be found, or the user pressed the
    // shift key while launching. Prompt to select the index.html file.
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    [openPanel setTitle:[NSString stringWithFormat:@"Please select the %@ index.html file", APP_NAME]];
    if ([openPanel runModal] == NSOKButton) {
      startupUrl = [[openPanel URLs] objectAtIndex:0];
    }
    else {
      // User chose cancel when selecting startup file. Exit.
      [NSApp terminate:nil];
      return 0;
    }
  }
  
  // Create the application delegate and window.
  [delegate performSelectorOnMainThread:@selector(createApp:) withObject:nil
                          waitUntilDone:NO];

  // Run the application message loop.
  CefRunMessageLoop();
    
  //if we quit the message loop programatically we need to call
  //terminate now to properly cleanup everything
  if (!g_isTerminating) {
    g_isTerminating = true;
    [NSApp terminate:NULL];
  }
  
  // Don't put anything below this line because it won't be executed.
  return 0;
}


// Global functions

std::string AppGetWorkingDirectory() {
  return szWorkingDir;
}

CefString AppGetCachePath() {
  std::string cachePath = std::string(ClientApp::AppGetSupportDirectory()) + "/cef_data";
  
  return CefString(cachePath);
}

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
