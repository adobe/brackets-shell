// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#include "client_handler.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/wrapper/cef_helpers.h"
#include "cefclient.h"

#import "PopupClientWindowDelegate.h"


// ClientHandler::ClientLifeSpanHandler implementation

void ClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  CEF_REQUIRE_UI_THREAD();

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
  CEF_REQUIRE_UI_THREAD();

  // Set the frame window title bar
  NSView* view = (NSView*)browser->GetHost()->GetWindowHandle();
  NSWindow* window = [view window];
  std::string titleStr(title);
  NSString* str = [NSString stringWithUTF8String:titleStr.c_str()];
  
  NSObject* delegate = [window delegate];
  if ([window styleMask] & NSFullScreenWindowMask) {
      [window setTitle:str];
  }
    // This is required as we are now managing the window's title
    // on our own, using CustomTitlbeBarView. As we are nuking the
    // window's title, currently open windows like new brackets
    // windows/dev tools are not getting listed under the
    // 'Window' menu. So we are now explicitly telling OS to make
    // sure we have a menu entry under 'Window' menu. This is a
    // safe call and is officially supported.
  [NSApp changeWindowsItem:window title:str filename:NO];

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

void ClientHandler::CloseMainWindow() {
  // TODO(port): Close window
}


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
    if (browser->GetFocusedFrame()->GetURL() == "chrome-devtools://devtools/inspector.html") {
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
