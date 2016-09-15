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

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include "client_handler.h"

@class CustomTitlebarView;

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
