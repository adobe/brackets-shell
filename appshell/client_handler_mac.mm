// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

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



extern CefRefPtr<ClientHandler> g_handler;

// Custom draw interface for NSThemeFrame
@interface NSView (UndocumentedAPI)
- (float)roundedCornerRadius;
- (CGRect)_titlebarTitleRect;
- (NSTextFieldCell*)titleCell;
- (void)_drawTitleStringIn:(struct CGRect)arg1 withColor:(id)color;
@end

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
  BOOL isReallyClosing;
  NSString* savedTitle;
}
- (IBAction)quit:(id)sender;
- (IBAction)handleMenuAction:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem;
- (BOOL)windowShouldClose:(id)window;
- (void)setClientHandler:(CefRefPtr<ClientHandler>)handler;
- (void)setWindow:(NSWindow*)window;
- (void)setFullScreenButtonView:(NSView*)view;
- (void)addCustomDrawHook:(NSView*)contentView;
- (BOOL)isFullScreenSupported;
@end


/**
 * The patched implementation for drawRect that lets us tweak
 * the title bar.
 */
void PopupWindowFrameDrawRect(id self, SEL _cmd, NSRect rect) {
    // Clear to 0 alpha
    [[NSColor clearColor] set];
    NSRectFill( rect );
    //Obtain reference to our NSThemeFrame view
    NSRect windowRect = [self frame];
    windowRect.origin = NSMakePoint(0,0);
    //This constant matches the radius for other macosx apps.
    //For some reason if we use the default value it is double that of safari etc.
    float cornerRadius = 4.0f;
    
    //Clip our title bar render
    [[NSBezierPath bezierPathWithRoundedRect:windowRect
                                     xRadius:cornerRadius
                                     yRadius:cornerRadius] addClip];
    [[NSBezierPath bezierPathWithRect:rect] addClip];
    
    
    
    NSColorSpace *sRGB = [NSColorSpace sRGBColorSpace];
    // Background fill, solid for now.
    NSColor *fillColor = [NSColor colorWithColorSpace:sRGB components:fillComp count:4];
    [fillColor set];
    NSRectFill( rect );
    NSColor *activeColor = [NSColor colorWithColorSpace:sRGB components:activeComp count:4];
    NSColor *inactiveColor = [NSColor colorWithColorSpace:sRGB components:inactiveComp count:4];
    // Render our title text
    [self _drawTitleStringIn:[self _titlebarTitleRect]
                   withColor:[NSApp isActive] ?
                activeColor : inactiveColor];
    
}


/**
 * Create a custom class based on NSThemeFrame called
 * ShellWindowFrame. ShellWindowFrame uses ShellWindowFrameDrawRect()
 * as the implementation for the drawRect selector allowing us
 * to draw the border/title bar the way we see fit.
 */
Class GetPopuplWindowFrameClass() {
    // lazily change the class implementation if
    // not done so already.
    static Class k = NULL;
    if (!k) {
        // See http://cocoawithlove.com/2010/01/what-is-meta-class-in-objective-c.html
        Class NSThemeFrame = NSClassFromString(@"NSThemeFrame");
        k = objc_allocateClassPair(NSThemeFrame, "PopupWindowFrame", 0);
        Method m0 = class_getInstanceMethod(NSThemeFrame, @selector(drawRect:));
        class_addMethod(k, @selector(drawRect:),
                        (IMP)PopupWindowFrameDrawRect, method_getTypeEncoding(m0));
        objc_registerClassPair(k);
    }
    return k;
}

@implementation PopupClientWindowDelegate

- (id) init {
  [super init];
  isReallyClosing = false;
  savedTitle = nil;
  fullScreenButtonView = nil;
  return self;
}

- (IBAction)quit:(id)sender {
  /*
  CefRefPtr<CefBrowser> browser;
  
  // If the main browser exists, send the command to that browser
  if (clientHandler->GetBrowserId())
    browser = clientHandler->GetBrowser();
  
  if (!browser)
    browser = ClientHandler::GetBrowserForNativeWindow(window);
  
  // TODO: we should have a "get frontmost brackets window" command for this
  
  if (clientHandler && browser) {
    clientHandler->SendJSCommand(browser, FILE_QUIT);
  } else {
    [NSApp terminate:nil];
  }
  */
  clientHandler->DispatchCloseToNextBrowser();
}

- (void)addCustomDrawHook:(NSView*)contentView
{
    NSView* themeView = [contentView superview];
    
    object_setClass(themeView, GetPopuplWindowFrameClass());
    
#ifdef LIGHT_CAPTION_TEXT
    // Reset our frame view text cell background style
    NSTextFieldCell * cell = [themeView titleCell];
    [cell setBackgroundStyle:NSBackgroundStyleLight];
#endif
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

- (BOOL)isFullScreenSupported {
  SInt32 version;
  Gestalt(gestaltSystemVersion, &version);
  return (version >= 0x1070);
}

- (void)setFullScreenButtonView:(NSView *)view {
  fullScreenButtonView = view;
}

-(void)windowTitleDidChange:(NSString*)title {
#ifdef DARK_UI
    savedTitle = [title copy];
#endif
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification {
#ifdef DARK_UI
    savedTitle = [[window title] copy];
    [window setTitle:@""];
#endif
}


-(void)windowDidResize:(NSNotification *)notification
{
#ifdef DARK_UI
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

- (void)windowDidExitFullScreen:(NSNotification *)notification {
#ifdef DARK_UI
    NSView* contentView = [window contentView];
    [self addCustomDrawHook: contentView];
    [window setTitle:savedTitle];
    [savedTitle release];
#endif
    
    NSView * themeView = [[window contentView] superview];
    NSRect  parentFrame = [themeView frame];
    NSWindow* theWin = window;
    NSButton *windowButton = nil;
    
#ifdef CUSTOM_TRAFFIC_LIGHTS
    //hide buttons
    windowButton = [theWin standardWindowButton:NSWindowCloseButton];
    [windowButton setHidden:YES];
    windowButton = [theWin standardWindowButton:NSWindowMiniaturizeButton];
    [windowButton setHidden:YES];
    windowButton = [theWin standardWindowButton:NSWindowZoomButton];
    [windowButton setHidden:YES];
    
    TrafficLightsViewController     *controller = [[TrafficLightsViewController alloc] init];
    
    if ([NSBundle loadNibNamed: @"TrafficLights" owner: controller])
    {
        NSRect  oldFrame = [controller.view frame];
        NSRect newFrame = NSMakeRect(kTrafficLightsViewX,	// x position
                                     parentFrame.size.height - oldFrame.size.height - kTrafficLightsViewY,   // y position
                                     oldFrame.size.width,                                  // width
                                     oldFrame.size.height);                                // height
        [controller.view setFrame:newFrame];
        [themeView addSubview:controller.view];
    }
#endif
    
#ifdef DARK_UI
    if ([self isFullScreenSupported]) {
        windowButton = [theWin standardWindowButton:NSWindowFullScreenButton];
        [windowButton setHidden:YES];
    
        FullScreenViewController     *fsController = [[FullScreenViewController alloc] init];
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
    
    [themeView setNeedsDisplay:YES];
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

void ClientHandler::PopupCreated(CefRefPtr<CefBrowser> browser) {
  NSWindow* window = [browser->GetHost()->GetWindowHandle() window];
  [window setCollectionBehavior: (1 << 7) /* NSWindowCollectionBehaviorFullScreenPrimary */];
  
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
#ifdef DARK_UI
      NSView* contentView = [window contentView];
      [delegate addCustomDrawHook: contentView];
#endif

      NSWindow* theWin = window;
      NSView*   themeView = [[window contentView] superview];
      NSRect    parentFrame = [themeView frame];
      NSButton* windowButton = nil;
      
#ifdef CUSTOM_TRAFFIC_LIGHTS
      windowButton = [theWin standardWindowButton:NSWindowCloseButton];
      [windowButton setHidden:YES];
      windowButton = [theWin standardWindowButton:NSWindowMiniaturizeButton];
      [windowButton setHidden:YES];
      windowButton = [theWin standardWindowButton:NSWindowZoomButton];
      [windowButton setHidden:YES];
      
      TrafficLightsViewController     *controller = [[TrafficLightsViewController alloc] init];
      
      if ([NSBundle loadNibNamed: @"TrafficLights" owner: controller])
      {
          NSRect  oldFrame = [controller.view frame];
          NSRect newFrame = NSMakeRect(kTrafficLightsViewX,	// x position
                                       parentFrame.size.height - oldFrame.size.height - 4,   // y position
                                       oldFrame.size.width,                                  // width
                                       oldFrame.size.height);                                // height
          [controller.view setFrame:newFrame];
          [themeView addSubview:controller.view];
      }
#endif
      
#ifdef DARK_UI
      if ([delegate isFullScreenSupported]) {
          windowButton = [theWin standardWindowButton:NSWindowFullScreenButton];
          [windowButton setHidden:YES];
      
          FullScreenViewController     *fsController = [[FullScreenViewController alloc] init];
          if ([NSBundle loadNibNamed: @"FullScreen" owner: fsController])
          {
              NSRect oldFrame = [fsController.view frame];
              NSRect newFrame = NSMakeRect(parentFrame.size.width - oldFrame.size.width - 4,	// x position
                                           parentFrame.size.height - oldFrame.size.height - kTrafficLightsViewY,   // y position
                                           oldFrame.size.width,                                  // width
                                           oldFrame.size.height);                                // height
              [fsController.view setFrame:newFrame];
              [themeView addSubview:fsController.view];
              [delegate setFullScreenButtonView:fsController.view];
          }
      }
#endif

      [themeView setNeedsDisplay:YES];
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
