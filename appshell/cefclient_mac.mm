// Copyright (c) 2010 The Chromium Embedded Framework Authors.
// Portions copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#include <sstream>
#include "cefclient.h"
#include "include/cef_app.h"
#include "include/cef_version.h"
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

#include "TrafficLightsView.h"
#include "TrafficLightsViewController.h"
#include "client_colors_mac.h"

#include "FullScreenView.h"
#include "FullScreenViewController.h"

#import "CustomTitlebarView.h"

// Application startup time
CFTimeInterval g_appStartupTime;

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;
static bool g_isTerminating = false;

char szWorkingDir[512];   // The current working directory

NSURL* startupUrl = nil;

// Content area size for newly created windows.
const int kWindowWidth = 1000;
const int kWindowHeight = 700;
const int kMinWindowWidth = 375;
const int kMinWindowHeight = 200;


// Memory AutoRelease pool.
static NSAutoreleasePool* g_autopool = nil;

// A list of files that will be opened by the app during startup.
// If this list is non-NULL, it means the app is not yet ready to open any files,
// so if you want to open a file you must add it to this list.
// If the list is NULL, that means that startup has finished,
// so if you want to open a file you need to call SendOpenFileCommand.
extern NSMutableArray* pendingOpenFiles;

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
  if ([event type] == NSKeyDown) {
    // If mainWindow is the first responder then cef isn't the target
    // so let the application event chain handle it intrinsically
    if ([[self mainWindow] firstResponder] == [self mainWindow] && 
	  [event modifierFlags] & NSCommandKeyMask) {
      // We've removed cut, copy, paste from the edit menu,
      // so we handle those shortcuts explicitly.
      SEL theSelector = nil;
      NSString *keyStr = [event charactersIgnoringModifiers];
      unichar keyChar = [keyStr characterAtIndex:0];
      if ( keyChar == 'c') {
        theSelector = NSSelectorFromString(@"copy:");
      } else if (keyChar == 'v'){
        theSelector = NSSelectorFromString(@"paste:");
      } else if (keyChar == 'x'){
        theSelector = NSSelectorFromString(@"cut:");
      } else if (keyChar == 'a'){
        theSelector = NSSelectorFromString(@"selectAll:");
      } else if (keyChar == 'z'){
        theSelector = NSSelectorFromString(@"undo:");
      } else if (keyChar == 'Z'){
        theSelector = NSSelectorFromString(@"redo:");
      }
      if (theSelector != nil) {
        [[NSApplication sharedApplication] sendAction:theSelector to:nil from:nil];
      }
    }
  }
  CefScopedSendingEvent sendingEventScoper;
  [super sendEvent:event];
}

// Find a window that can handle an openfile command.
- (NSWindow *) findTargetWindow {
  NSApplication * nsApp = (NSApplication *)self;
  NSWindow* result = [nsApp keyWindow];
  if (!result) {
    result = [nsApp mainWindow];
    if (!result) {
      // the app might be inactive or hidden. Look for the main window on the window list.
      NSArray* windows = [nsApp windows];
      for (NSUInteger i = 0; i < [windows count]; i++) {
        NSWindow* window = [windows objectAtIndex:i];

        // Note: this only finds the main (first) appshell window. If additional
        // windows are open, they will _not_ be found. If the main (first) window
        // is closed, it may be found, but will be hidden.
        if ([[window frameAutosaveName] isEqualToString: APP_NAME @"MainWindow"]) {
          result = window;
          break;
        }
      }
    }
  }
  return result;
}

@end

// BOBNOTE: Consider moving the delegate interface into its own .h file
@interface ClientMenuDelegate : NSObject <NSMenuDelegate> {
}
- (void)menuWillOpen:(NSMenu *)menu;
@end

@implementation ClientMenuDelegate

- (void)menuWillOpen:(NSMenu *)menu {
    // Notify that menu is being popped up
    NSWindow* targetWindow = [NSApp findTargetWindow];
    if (targetWindow) {
        CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow(targetWindow);
        if (browser)
            g_handler->SendJSCommand(browser, APP_BEFORE_MENUPOPUP);
    }
}

@end

// BOBNOTE: Consider moving the delegate interface into its own .h file
// Receives notifications from controls and the browser window. Will delete
// itself when done.
@interface ClientWindowDelegate : NSObject <NSWindowDelegate> {
    BOOL isReallyClosing;
    NSView* fullScreenButtonView;
    NSView* trafficLightsView;
    BOOL  isReentering;
    CustomTitlebarView  *customTitlebar;
}
- (void)setIsReallyClosing;
- (IBAction)handleMenuAction:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem;
- (IBAction)showAbout:(id)sender;
- (IBAction)quit:(id)sender;
- (void)alert:(NSString*)title withMessage:(NSString*)message;
- (void)notifyConsoleMessage:(id)object;
- (void)notifyDownloadComplete:(id)object;
- (void)notifyDownloadError:(id)object;
- (void)setFullScreenButtonView:(NSView*)view;
- (void)setTrafficLightsView:(NSView*)view;
@end

@implementation ClientWindowDelegate
- (id) init {
    self = [super init];
    isReallyClosing = NO;
    isReentering = NO;
    customTitlebar = nil;
    fullScreenButtonView = nil;
    trafficLightsView = nil;
    return self;
}

- (void)setIsReallyClosing {
  isReallyClosing = true;
}

- (IBAction)showAbout:(id)sender {
  if (g_handler.get() && g_handler->GetBrowserId())
    g_handler->SendJSCommand(g_handler->GetBrowser(), HELP_ABOUT);
}

-(IBAction)handleOpenPreferences:(id)sender {
    g_handler->SendJSCommand(g_handler->GetBrowser(), OPEN_PREFERENCES_FILE);
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


-(void)setFullScreenButtonView:(NSView *)view {
    fullScreenButtonView = view;
}


-(void)setTrafficLightsView:(NSView *)view {
    trafficLightsView = view;
}

-(void)windowTitleDidChange:(NSString*)title {
#ifdef DARK_UI
    if (customTitlebar) {
        [customTitlebar setTitleString:title];
    }
#endif
}

-(BOOL)isRunningOnYosemite {
    NSDictionary* dict = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
    NSString* version =  [dict objectForKey:@"ProductVersion"];
    return [version hasPrefix:@"10.10"];
}

- (BOOL)isFullScreenSupported {
    // Return False on Yosemite so we
    //  don't draw our own full screen button
    //  and handle full screen mode
    if (![self isRunningOnYosemite]) {
        SInt32 version;
        Gestalt(gestaltSystemVersion, &version);
        return (version >= 0x1070);
    }
    return false;
}

-(BOOL)needsFullScreenActivateHack {
    if (![self isRunningOnYosemite]) {
        SInt32 version;
        Gestalt(gestaltSystemVersion, &version);
        return (version >= 0x1090);
    }
    return false;
}

-(BOOL)useSystemTrafficLights {
    return [self isRunningOnYosemite];
}

-(void)windowDidResize:(NSNotification *)notification
{

// BOBNOTE: this should be moved into the CustomTitlebarView class
#ifdef DARK_UI
    NSWindow* window = [notification object];

    if ([self isFullScreenSupported]) {
        
        NSView* themeView = [[window contentView] superview];
        NSRect  parentFrame = [themeView frame];
        
        NSRect oldFrame = [fullScreenButtonView frame];
        NSRect newFrame = NSMakeRect(parentFrame.size.width - oldFrame.size.width - 4,	// x position
                                     parentFrame.size.height - oldFrame.size.height - kTrafficLightsViewY,   // y position
                                     oldFrame.size.width,                                  // width
                                     oldFrame.size.height);
        
        [fullScreenButtonView setFrame:newFrame];
        [themeView setNeedsDisplay:YES];
    }
#endif
}


- (void)windowWillEnterFullScreen:(NSNotification *)notification {
#ifdef DARK_UI
    if (fullScreenButtonView) {
        [fullScreenButtonView removeFromSuperview];
        fullScreenButtonView = nil;
    }
    if (trafficLightsView) {
        [trafficLightsView setHidden:YES];
    }
    if (customTitlebar) {
        [customTitlebar setHidden:YES];
    }
    
    if ([self needsFullScreenActivateHack]) {
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp unhide:nil];
        NSWindow* window = [notification object];
        NSView* contentView = [window contentView];
        [contentView setNeedsDisplay:YES];
    }
#endif
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification {
#ifdef DARK_UI
    if ([self needsFullScreenActivateHack]) {
        NSWindow* window = [notification object];
        NSView* contentView = [window contentView];
        
        [contentView setNeedsDisplay:YES];
    }
#endif
}

-(void)initUI:(NSWindow*)mainWindow {
    NSView* contentView = [mainWindow contentView];
    NSView* themeView = [contentView superview];
    NSRect  parentFrame = [themeView frame];
    NSButton *windowButton = nil;
    
#ifdef CUSTOM_TRAFFIC_LIGHTS
    if (![self useSystemTrafficLights] && !trafficLightsView) {
        windowButton = [mainWindow standardWindowButton:NSWindowCloseButton];
        [windowButton setHidden:YES];
        windowButton = [mainWindow standardWindowButton:NSWindowMiniaturizeButton];
        [windowButton setHidden:YES];
        windowButton = [mainWindow standardWindowButton:NSWindowZoomButton];
        [windowButton setHidden:YES];
        
        TrafficLightsViewController     *tvController = [[[TrafficLightsViewController alloc] init] autorelease];
        if ([NSBundle loadNibNamed: @"TrafficLights" owner: tvController])
        {
            NSRect oldFrame = [tvController.view frame];
            NSRect newFrame = NSMakeRect(kTrafficLightsViewX,	// x position
                                         parentFrame.size.height - oldFrame.size.height - kTrafficLightsViewY,   // y position
                                         oldFrame.size.width,                                  // width
                                         oldFrame.size.height);                                // height
            [tvController.view setFrame:newFrame];
            [themeView addSubview:tvController.view];
            [self setTrafficLightsView:tvController.view];
        }
    }
    
#endif
#ifdef DARK_UI
    if ([self isFullScreenSupported] && !fullScreenButtonView) {
        windowButton = [mainWindow standardWindowButton:NSWindowFullScreenButton];
        [windowButton setHidden:YES];
        
        FullScreenViewController     *fsController = [[[FullScreenViewController alloc] init] autorelease];
        if ([NSBundle loadNibNamed: @"FullScreen" owner: fsController])
        {
            NSRect oldFrame = [fsController.view frame];
            NSRect newFrame = NSMakeRect(parentFrame.size.width - oldFrame.size.width - 4,	// x position
                                         parentFrame.size.height - oldFrame.size.height - kTrafficLightsViewY,   // y position
                                         oldFrame.size.width,                                  // width
                                         oldFrame.size.height);                                // height
            [fsController.view setFrame:newFrame];
            [themeView addSubview:fsController.view];
            [self setFullScreenButtonView:fsController.view];
        }
    }
#endif
    

}

- (void)windowDidExitFullScreen:(NSNotification *)notification {
    // This effectively recreates the full screen button in it's default \
    // state.  Don't do this until after animation has completed or it will
    // be in the wrong state and look funny...
    NSWindow* window = [notification object];
    [self initUI:window];
}


-(void)windowWillExitFullScreen:(NSNotification *)notification {
    // show the buttons and title bar so they appear during the
    //  transition from fullscreen back to normal
    if (customTitlebar) {
        [customTitlebar setHidden:NO];
    }
    if (trafficLightsView) {
        [trafficLightsView setHidden:NO];
    }
}

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
#ifdef DARK_UI
    if (!isReentering)
    {
        NSWindow    *thisWindow = [notification object];
        NSView*     contentView = [thisWindow contentView];
        NSRect      bounds = [[contentView superview] bounds];

        customTitlebar = [[CustomTitlebarView alloc] initWithFrame:bounds];
        
        [customTitlebar setTitleString: [thisWindow title]];

        [customTitlebar setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
        [[contentView superview] addSubview:customTitlebar positioned:NSWindowBelow relativeTo:[[[contentView superview] subviews] objectAtIndex:0]];
        
        NSButton    *windowButton = [thisWindow standardWindowButton:NSWindowFullScreenButton];
        [windowButton setHidden:YES];
        isReentering = YES;
    }
#endif
    
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

// BOBNOTE: Consider moving the AppDelegate interface into its own .h file
// Receives notifications from the application. Will delete itself when done.
@interface ClientAppDelegate : NSObject
{
    ClientWindowDelegate *delegate;
    ClientMenuDelegate *menuDelegate;
}

@property (nonatomic, retain) ClientWindowDelegate *delegate;
@property (nonatomic, retain) ClientMenuDelegate *menuDelegate;

- (void)createApp:(id)object;
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (BOOL)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames;
@end


// BOBNOTE: Consider moving the AppDelegate implementation into its own .m file
@implementation ClientAppDelegate
@synthesize delegate, menuDelegate;

- (id) init {
  self = [super init];
  // Register our handler for the "handleOpenFileEvent" (a.k.a. OpFl) apple event.
  [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
                                                     andSelector:@selector(handleOpenFileEvent:withReplyEvent:)
                                                   forEventClass:'aevt'
                                                      andEventID:'OpFl'];
  return self;
}

- (void)dealloc {
    [self.delegate release];
    [self.menuDelegate release];
    [super dealloc];
}

// Create the application on the UI thread.
- (void)createApp:(id)object {
  [NSApplication sharedApplication];
  [NSBundle loadNibNamed:@"MainMenu" owner:NSApp];
  
  // Set the delegate for application events.
  [NSApp setDelegate:self];
  
  // Create the delegate for control and browser window events.
  [self setDelegate:[[ClientWindowDelegate alloc] init]];

  // Create the delegate for menu events.
  [self setMenuDelegate:[[ClientMenuDelegate alloc] init]];

  [[NSApp mainMenu] setDelegate:self.menuDelegate];
  [[[[NSApp mainMenu] itemWithTag: BRACKETS_MENUITEMTAG] submenu] setDelegate:self.menuDelegate];
  [[[[NSApp mainMenu] itemWithTag: WINDOW_MENUITEMTAG]   submenu] setDelegate:self.menuDelegate];

  // Create the main application window.
  NSUInteger styleMask = (NSTitledWindowMask |
                          NSClosableWindowMask |
                          NSMiniaturizableWindowMask |
                          NSResizableWindowMask |
                          NSTexturedBackgroundWindowMask );

  // Get the available screen space
  NSRect screen_rect = [[NSScreen mainScreen] visibleFrame];
  // Start out with the content being as big as possible
  NSRect content_rect = [NSWindow contentRectForFrameRect:screen_rect styleMask:styleMask];
  
  // Determine the maximum height
  const int maxHeight = kWindowHeight;
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

#ifdef DARK_UI
  NSColorSpace *sRGB = [NSColorSpace sRGBColorSpace];
  // Background fill, solid for now.
  NSColor *fillColor = [NSColor colorWithColorSpace:sRGB components:fillComp count:4];
  [mainWnd setMinSize:NSMakeSize(kMinWindowWidth, kMinWindowHeight)];
  [mainWnd setBackgroundColor:fillColor];
#endif
    
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
  [mainWnd setDelegate:self.delegate];
  [mainWnd setCollectionBehavior: (1 << 7) /* NSWindowCollectionBehaviorFullScreenPrimary */];

  // Rely on the window delegate to clean us up rather than immediately
  // releasing when the window gets closed. We use the delegate to do
  // everything from the autorelease pool so the window isn't on the stack
  // during cleanup (ie, a window close from javascript).
  [mainWnd setReleasedWhenClosed:NO];

  NSView* contentView = [mainWnd contentView];
	
  // Create the handler.
  g_handler = new ClientHandler();
  g_handler->SetMainHwnd(contentView);
	
  // Create the browser view.
  CefWindowInfo window_info;
  CefBrowserSettings settings;

  settings.web_security = STATE_DISABLED;

  CefRefPtr<CefCommandLine> cmdLine = AppGetCommandLine();

#ifdef DARK_INITIAL_PAGE
  // Avoid white flash at startup or refresh by making this the default
  // CSS.
  // 'aHRtbCxib2R5e2JhY2tncm91bmQ6cmdiYSgxMDksIDExMSwgMTEyLCAxKTt9' stands for 'html,body{background:rgba(109, 111, 112, 1);}'.
  const char* strCss = "data:text/css;charset=utf-8;base64,aHRtbCxib2R5e2JhY2tncm91bmQ6cmdiYSgxMDksIDExMSwgMTEyLCAxKTt9";
  CefString(&settings.user_style_sheet_location).FromASCII(strCss);
#endif
    
  window_info.SetAsChild(contentView, 0, 0, content_rect.size.width, content_rect.size.height);
  

    
  NSString* str = [[startupUrl absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
  CefBrowserHost::CreateBrowserSync(window_info, g_handler.get(),
                                [str UTF8String], settings, nil);
 
  [self.delegate initUI:mainWnd];
    
  // Show the window.
  [mainWnd display];
  [mainWnd makeKeyAndOrderFront: nil];
  [NSApp requestUserAttention:NSInformationalRequest];
  [NSApp unhide:nil];
}



// Handle the Openfile apple event. This is a custom apple event similar to the regular
// Open event, but can handle file paths in the form "path[:lineNumber[:columnNumber]]".
- (void)handleOpenFileEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent {
  NSAppleEventDescriptor* filePathDescriptor = [event paramDescriptorForKeyword:'file'];
  [self application:NSApp openFile:[filePathDescriptor stringValue]];
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
  if (!pendingOpenFiles) {
    ClientApplication * clientApp = (ClientApplication *)theApplication;
    NSWindow* targetWindow = [clientApp findTargetWindow];
    if (targetWindow) {
      CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow(targetWindow);
      g_handler->SendOpenFileCommand(browser, CefString([filename UTF8String]));
    }
  } else {
    // App is just starting up. Save the filename so we can open it later.
    [pendingOpenFiles addObject:filename];
  }
  return YES;
}

- (BOOL)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames {
  if (!pendingOpenFiles) {
    ClientApplication * clientApp = (ClientApplication *)theApplication;
    NSWindow* targetWindow = [clientApp findTargetWindow];
    if (targetWindow) {
      CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow(targetWindow);
      NSUInteger count = [filenames count];
      if (count) {
        std::string files = "[";
        for (NSUInteger i = 0; i < count; i++) {
          if (i > 0) {
            files += ", ";
          }
          files += ("\"" + std::string([[filenames objectAtIndex:i] UTF8String]) + "\"");
        }
        files += "]";
        g_handler->SendOpenFileCommand(browser, CefString(files));
      }
    }
  } else {
    // App is just starting up. Save the filenames so we can open them later.
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

  pendingOpenFiles = [[NSMutableArray alloc] init];
  
  CefMainArgs main_args(argc, argv);
 
  // Delete Special Characters Palette from Edit menu.
  [[NSUserDefaults standardUserDefaults]
    setBool:YES forKey:@"NSDisabledCharacterPaletteMenuItem"];
    
  g_appStartupTime = CFAbsoluteTimeGetCurrent();

  // Start the node server process
  startNodeProcess();
    
  CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app.get(), NULL);
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

  settings.no_sandbox = YES;
    
  // Check command
  if (CefString(&settings.cache_path).length() == 0) {
	  CefString(&settings.cache_path) = AppGetCachePath();
  }

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get(), NULL);

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
                          waitUntilDone:YES];

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

CefString AppGetChromiumVersionString() {
  NSMutableString *s = [NSMutableString stringWithFormat:@"Chrome/%d.%d.%d.%d",
                           cef_version_info(2), cef_version_info(3),
                           cef_version_info(4), cef_version_info(5)];
  CefString result = CefString([s UTF8String]);
  return result;
}
