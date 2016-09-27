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

#import "PopupClientWindowDelegate.h"

#import "CustomTitlebarView.h"

#include "client_colors_mac.h"
#include "config.h"
#include "native_menu_model.h"
#include "TrafficLightsView.h"
#include "TrafficLightsViewController.h"
#include "FullScreenView.h"
#include "FullScreenViewController.h"

// If app is built with 10.9 or lower
// this constant is not defined.
#ifndef NSAppKitVersionNumber10_10
    #define NSAppKitVersionNumber10_10 1343
#endif


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

-(BOOL)isRunningOnYosemiteOrLater {
    // This seems to be a more reliable way of checking
    // the MACOS version. Documentation about this available
    // at the following link.
    // https://developer.apple.com/library/mac/documentation/DeveloperTools/Conceptual/cross_development/Using/using.html

    if (NSAppKitVersionNumber >= NSAppKitVersionNumber10_10)
        return true;
    else
        return false;
}

- (BOOL)isFullScreenSupported {
    // Return False on Yosemite so we
    //  don't draw our own full screen button
    //  and handle full screen mode
    if (![self isRunningOnYosemiteOrLater]) {
        SInt32 version;
        Gestalt(gestaltSystemVersion, &version);
        return (version >= 0x1070);
    }
    return false;
}

-(BOOL)needsFullScreenActivateHack {
    if (![self isRunningOnYosemiteOrLater]) {
        SInt32 version;
        Gestalt(gestaltSystemVersion, &version);
        return (version >= 0x1090);
    }
    return false;
}

-(BOOL)useSystemTrafficLights {
    return [self isRunningOnYosemiteOrLater];
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

        NSWindow *popUpWindow = [notification object];

        // Since we have nuked the title, we will have
        // to set the string back as we are hiding the
        // custom title bar.
        [popUpWindow setTitle:@""];
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

        NSWindow *popUpWindow = [notification object];

        // Since we have nuked the title, we will have
        // to set the string back as we are hiding the
        // custom title bar.
        [popUpWindow setTitle:[customTitlebar titleString]];
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
