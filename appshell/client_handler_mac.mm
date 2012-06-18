// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "client_handler.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "cefclient.h"

extern CefRefPtr<ClientHandler> g_handler;

// ClientHandler::ClientLifeSpanHandler implementation

bool ClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
                                  const CefPopupFeatures& popupFeatures,
                                  CefWindowInfo& windowInfo,
                                  const CefString& url,
                                  CefRefPtr<CefClient>& client,
                                  CefBrowserSettings& settings) {
  REQUIRE_UI_THREAD();

  return false;
}

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
@interface PopupClientWindowDelegate : NSObject <NSWindowDelegate>
- (IBAction)quit:(id)sender;
- (BOOL)windowShouldClose:(id)window;
- (void)setClientHandler:(CefRefPtr<ClientHandler>)handler;
- (void)setWindow:(NSWindow*)window;

CefRefPtr<ClientHandler> clientHandler;
NSWindow* window;
@end

@implementation PopupClientWindowDelegate

- (IBAction)quit:(id)sender {
  CefRefPtr<CefBrowser> browser;
  
  // If the main browser exists, send the command to that browser
  if (clientHandler->GetBrowserId())
    browser = clientHandler->GetBrowser();
  else
    browser = ClientHandler::GetBrowserForNativeWindow(window);
  
  // TODO: we should have a "get frontmost brackets window" command for this
  
  if (clientHandler && browser) {
    clientHandler->SendJSCommand(browser, FILE_QUIT);
  } else {
    [NSApp terminate:nil];
  }
}

- (void)setClientHandler:(CefRefPtr<ClientHandler>) handler {
  clientHandler = handler;
}

- (void)setWindow:(NSWindow*)win {
  window = win;
}

// Called when the window is about to close. Perform the self-destruction
// sequence by getting rid of the window. By returning YES, we allow the window
// to be removed from the screen.

- (BOOL)windowShouldClose:(id)window {  
  CefRefPtr<CefBrowser> browser = ClientHandler::GetBrowserForNativeWindow(window);
  
  if (browser) {
    CefRefPtr<CommandCallback> callback = new CloseWindowCommandCallback(browser);

    clientHandler->SendJSCommand(browser, FILE_CLOSE_WINDOW, callback);
    return NO;
  }
  
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
  
  if (![window delegate]) {
    PopupClientWindowDelegate* delegate = [[PopupClientWindowDelegate alloc] init];
    [delegate setClientHandler:this];
    [delegate setWindow:window];
    [window setDelegate:delegate];
  }
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
