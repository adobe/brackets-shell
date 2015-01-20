// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
#define OS_MAC

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#include "client_handler.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "cefclient.h"
#include "native_menu_model.h"

#include "TrafficLightsView.h"
#include "TrafficLightsViewController.h"

#include "FullScreenView.h"
#include "FullScreenViewController.h"

#include "config.h"
#include "client_colors_mac.h"
#import "CustomTitlebarView.h"


extern CefRefPtr<ClientHandler> g_handler;


// ClientHandler::ClientLifeSpanHandler implementation

void ClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
    // Set the edit window text
    NSTextField* textField = (NSTextField*)m_EditHwnd;
    std::string urlStr(url);
    NSString* str = [NSString stringWithUTF8String:urlStr.c_str()];
    [textField setStringValue:str];
  }
}

void ClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  REQUIRE_UI_THREAD();

  // Set the frame window title bar
  NSView* view = (NSView*)browser->GetHost()->GetWindowHandle();
  NSWindow* window = [view window];
  std::string titleStr(title);
  NSString* str = [NSString stringWithUTF8String:titleStr.c_str()];
  [window setTitle:str];
    
  NSObject* delegate = [window delegate];
  [delegate performSelectorOnMainThread:@selector(windowTitleDidChange:) withObject:str waitUntilDone:NO];
}

void ClientHandler::SendNotification(NotificationType type) {
  SEL sel = nil;
  switch(type) {
    case NOTIFY_CONSOLE_MESSAGE:
      sel = @selector(notifyConsoleMessage:);
      break;
    case NOTIFY_DOWNLOAD_COMPLETE:
      sel = @selector(notifyDownloadComplete:);
      break;
    case NOTIFY_DOWNLOAD_ERROR:
      sel = @selector(notifyDownloadError:);
      break;
  }

  if (sel == nil)
    return;

  NSWindow* window = [AppGetMainHwnd() window];
  NSObject* delegate = [window delegate];
  [delegate performSelectorOnMainThread:sel withObject:nil waitUntilDone:NO];
}

void ClientHandler::SetLoading(bool isLoading) {
  // TODO(port): Change button status.
}

void ClientHandler::SetNavState(bool canGoBack, bool canGoForward) {
  // TODO(port): Change button status.
}

void ClientHandler::CloseMainWindow() {
  // TODO(port): Close window
}


// Receives notifications from controls and the browser window. Will delete
// itself when done.
@interface PopupClientWindowDelegate : NSObject <NSWindowDelegate> {
  CefRefPtr<ClientHandler> clientHandler;
  NSWindow* window;
  NSView* fullScreenButtonView;
  NSView* trafficLightsView;
  BOOL isReallyClosing;
  CustomTitlebarView  *customTitlebar;
}
- (IBAction)quit:(id)sender;
- (IBAction)handleMenuAction:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem;
- (BOOL)windowShouldClose:(id)window;
- (void)setClientHandler:(CefRefPtr<ClientHandler>)handler;
- (void)setWindow:(NSWindow*)window;
- (void)setFullScreenButtonView:(NSView*)view;
- (void)setTrafficLightsView:(NSView*)view;
- (BOOL)isFullScreenSupported;
- (void)makeDark;
- (void)initUI;
@end



@implementation PopupClientWindowDelegate

- (id) init {
  self = [super init];
  isReallyClosing = NO;
  fullScreenButtonView = nil;
  customTitlebar = nil;
  trafficLightsView = nil;
  return self;
}

- (IBAction)quit:(id)sender {
  clientHandler->DispatchCloseToNextBrowser();
}

- (IBAction)handleMenuAction:(id)sender {
    if (clientHandler.get() && clientHandler->GetBrowserId()) {
        CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow(window);
        NSMenuItem* senderItem = sender;
        NSUInteger tag = [senderItem tag];
        ExtensionString commandId = NativeMenuModel::getInstance(getMenuParent(browser)).getCommandId(tag);
        CefRefPtr<CommandCallback> callback = new EditCommandCallback(browser, commandId);
        clientHandler->SendJSCommand(browser, commandId, callback);
    }
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow(window);
    NSInteger menuState = NSOffState;
    NSUInteger tag = [menuItem tag];
    NativeMenuModel menus = NativeMenuModel::getInstance(getMenuParent(browser));
    if (menus.isMenuItemChecked(tag)) {
        menuState = NSOnState;
    }
    [menuItem setState:menuState];
    return menus.isMenuItemEnabled(tag);
}

- (void)setClientHandler:(CefRefPtr<ClientHandler>) handler {
  clientHandler = handler;
}

- (void)setWindow:(NSWindow*)win {
  window = win;
}

- (void)setIsReallyClosing {
  isReallyClosing = true;
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

-(void)makeDark {
    if (!customTitlebar)
    {
        NSView*     contentView = [window contentView];
        NSRect      bounds = [[contentView superview] bounds];
        
        customTitlebar = [[CustomTitlebarView alloc] initWithFrame:bounds];
        
        [customTitlebar setTitleString: [window title]];
        
        [customTitlebar setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
        [[contentView superview] addSubview:customTitlebar positioned:NSWindowBelow relativeTo:[[[contentView superview] subviews] objectAtIndex:0]];
    }
}


-(void)windowDidResize:(NSNotification *)notification
{
#ifdef DARK_UI
    if ([self isFullScreenSupported] && fullScreenButtonView) {
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

- (void)initUI {
    NSWindow* theWin = window;
    NSView*   themeView = [[window contentView] superview];
    NSRect    parentFrame = [themeView frame];
    NSButton* windowButton = nil;
    
#ifdef CUSTOM_TRAFFIC_LIGHTS
    if (![self useSystemTrafficLights] && !trafficLightsView) {
        windowButton = [theWin standardWindowButton:NSWindowCloseButton];
        [windowButton setHidden:YES];
        windowButton = [theWin standardWindowButton:NSWindowMiniaturizeButton];
        [windowButton setHidden:YES];
        windowButton = [theWin standardWindowButton:NSWindowZoomButton];
        [windowButton setHidden:YES];
        
        TrafficLightsViewController     *tlController = [[[TrafficLightsViewController alloc] init] autorelease];
        
        if ([NSBundle loadNibNamed: @"TrafficLights" owner: tlController])
        {
            NSRect  oldFrame = [tlController.view frame];
            NSRect newFrame = NSMakeRect(kTrafficLightsViewX,	// x position
                                         parentFrame.size.height - oldFrame.size.height - 4,   // y position
                                         oldFrame.size.width,                                  // width
                                         oldFrame.size.height);                                // height
            [tlController.view setFrame:newFrame];
            [themeView addSubview:tlController.view];
            [self setTrafficLightsView:tlController.view];
        }
    }
#endif
    
#ifdef DARK_UI
    if ([self isFullScreenSupported] && !fullScreenButtonView) {
        windowButton = [theWin standardWindowButton:NSWindowFullScreenButton];
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
    [self makeDark];
#endif

    
}

-(void)windowWillExitFullScreen:(NSNotification *)notification {
    // unhide these so they appear as the window
    //  transforms from full screen back to normal
    if (customTitlebar) {
        [customTitlebar setHidden:NO];
    }
    if (trafficLightsView) {
        [trafficLightsView setHidden:NO];
    }
}


-(void)windowDidExitFullScreen:(NSNotification *)notification {
    // This effectively recreates the fs button
    //  but we have to wait until after the animation
    //  is complete to create the button.  it will display
    //  in the wrong state if we do it sooner
    [self initUI];
}


- (void)windowWillEnterFullScreen:(NSNotification *)notification {
#ifdef DARK_UI
    // hide all of the elements so the os can make our
    //  window's content view can take up the entire display surface
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
        // HACK  to make sure that window is activate
        //  when going into full screen mode
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp unhide:nil];
        NSView* contentView = [window contentView];
        [contentView setNeedsDisplay:YES];
    }
#endif
}


- (void)windowDidEnterFullScreen:(NSNotification *)notification {
#ifdef DARK_UI
    if ([self needsFullScreenActivateHack]) {
        // HACK  to make sure that window is activate
        //  when going into full screen mode
        NSView* contentView = [window contentView];
        [contentView setNeedsDisplay:YES];
    }
#endif
}

// Called when the window is about to close. Perform the self-destruction
// sequence by getting rid of the window. By returning YES, we allow the window
// to be removed from the screen.
- (BOOL)windowShouldClose:(id)aWindow {
  
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

  CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow(aWindow);
  
  if (!isReallyClosing && browser) {
    CefRefPtr<CommandCallback> callback = new CloseWindowCommandCallback(browser);

    clientHandler->SendJSCommand(browser, FILE_CLOSE_WINDOW, callback);
    return NO;
  }
    
  // Clean ourselves up after clearing the stack of anything that might have the
  // window on it.
  [(NSObject*)[window delegate] performSelectorOnMainThread:@selector(cleanup:)
                                                 withObject:aWindow  waitUntilDone:NO];

  return YES;
}

// Deletes itself.
- (void)cleanup:(id)window {  
  [self release];
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
#ifdef DARK_UI
    [self makeDark];
#endif
    
  CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow([notification object]);
  if(browser) {
    // Give focus to the browser window.
    browser->GetHost()->SetFocus(true);
  }
}

- (void)windowDidResignKey:(NSNotification *)notification {
  CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow([notification object]);
  if(browser) {
    // Remove focus from the browser window.
    browser->GetHost()->SetFocus(false);
  }
  [[notification object] makeFirstResponder:nil];
}
@end

static bool centerMe = false;

void ClientHandler::ComputePopupPlacement(CefWindowInfo& windowInfo)
{
    // CEF will transform the Y value returned here and use it to create
    //  the window which ends up placing the window not where we want it
    // Just set a flag to center the window using cocoa apis in PopupCreated
    centerMe = true;
}


void ClientHandler::PopupCreated(CefRefPtr<CefBrowser> browser) {
  NSWindow* window = [browser->GetHost()->GetWindowHandle() window];
    
  [window setCollectionBehavior: (1 << 7) /* NSWindowCollectionBehaviorFullScreenPrimary */];

    
  if (centerMe) {
#ifdef _USE_COCOA_CENTERING
      // Center on the display
      [window center];
#else
      // Center within the main window
      NSWindow* mainWindow = [m_MainHwnd window];
      NSRect rect = [mainWindow frame];
      NSRect windowRect = [window frame];
      
      float mW = rect.size.width;
      float mH = rect.size.height;
      float cW = windowRect.size.width;
      float cH = windowRect.size.height;
      
      windowRect.origin.x = (rect.origin.x + (mW /2)) - (cW / 2);
      windowRect.origin.y = (rect.origin.y + (mH /2)) - (cH / 2);

      [window setFrame:windowRect display:YES];
#endif
      centerMe = false;
  }

  
  // CEF3 is now using a window delegate with this revision http://code.google.com/p/chromiumembedded/source/detail?r=1149
  // And the declaration of the window delegate (CefWindowDelegate class) is in libcef/browser/browser_host_impl_mac.mm and
  // thus it is not available for us to extend it. So for now, we have to override it with our delegate class
  // (PopupClientWindowDelegate) since we're not using its implementation of JavaScript 'onbeforeunload' handling yet.
  // Besides, we need to keep using our window delegate class for native menus and commands to function correctly in all
  // popup windows. Ideally, we should extend CefWindowDelegate class when its declaration is available and start using its
  // windowShouldClose method instead of our own that is also handling similar events of JavaScript 'onbeforeunload' events.
  if ([window delegate]) {
      [[window delegate] release];  // Release the delegate created by CEF
      [window setDelegate: nil];
  }

  if (![window delegate]) {
      PopupClientWindowDelegate* delegate = [[PopupClientWindowDelegate alloc] init];
      [delegate setClientHandler:this];
      [delegate setWindow:window];
      [window setDelegate:delegate];
      [delegate initUI];
  }

}

bool ClientHandler::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                                  const CefKeyEvent& event,
                                  CefEventHandle os_event,
                                  bool* is_keyboard_shortcut) {
    
    //This is not fun. Here's the facts as I know them:
    //+ OnPreKeyEvent is before the browser get the keydown event
    //+ OnKeyEvent is after the browser has done it's default processing
    //+ On win the native keyboard accelerators fire in this function
    //+ On Mac if nothing else stops the event, the acceleators fire last
    //+ In JS if eventPreventDefault() is called, the shortcut won't fire on mac
    //+ On Win TranslateAccerator will return true even if the command is disabled
    //+ On Mac performKeyEquivalent will return true only of the command is enabled
    //+ On Mac if JS handles the command then the menu bar won't blink
    //+ In this fuction, the browser passed won't return a V8 Context so you can't get any access there
    //+ Communicating between the shell and JS is async, so there's no easy way for the JS to decided what to do
    //    in the middle of the key event, unless we introduce promises there, but that is a lot of work now
    
    
    // CEF 1750 -- We need to not handle keys for the DevTools Window.
    if (browser->GetFocusedFrame()->GetURL() == "chrome-devtools://devtools/devtools.html") {
        return false;
    }
    
    if([[NSApp mainMenu] performKeyEquivalent: os_event]) {
        return true;
    }
    return false;
}


bool ClientHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                               const CefKeyEvent& event,
                               CefEventHandle os_event) {
    return false;
}

CefRefPtr<CefBrowser> ClientHandler::GetBrowserForNativeWindow(void* window) {
  CefRefPtr<CefBrowser> browser = NULL;
  
  if (window) {
    //go through all the browsers looking for a browser within this window
    for (BrowserWindowMap::const_iterator i = browser_window_map_.begin() ; i != browser_window_map_.end() && browser == NULL ; i++ ) {
      NSView* browserView = i->first;
      if (browserView && [browserView window] == window) {
        browser = i->second;
      }
    }
  }
  return browser;
}



bool ClientHandler::CanCloseBrowser(CefRefPtr<CefBrowser> browser) {
  return true;
}
