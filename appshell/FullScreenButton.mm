/*
 * Copyright (c) 2013 Adobe Systems Incorporated. All rights reserved.
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
#import "FullScreenButton.h"

//these are defined in MainMainu.xib file
@implementation FullScreenButton {
    NSImage *inactive;
    NSImage *active;
    NSImage *hover;
    NSImage *pressed;
    BOOL activeState;
    BOOL hoverState;
    BOOL pressedState;
}

-(id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self setup];
    }
    return self;
}

- (id)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        [self setup];
    }
    return self;
}

- (void)setup {
    inactive = [NSImage imageNamed:@"window-fullscreen-inactive"];
    active = [NSImage imageNamed:@"window-fullscreen-active"];
    hover = [NSImage imageNamed:@"window-fullscreen-hover"];
    pressed = [NSImage imageNamed:@"window-fullscreen-pressed"];

    // assume active
    activeState = YES;
    [self updateButtonStates];

    //get notified of state
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(updateActiveState)
                                                 name:NSWindowDidBecomeMainNotification object:[self window]];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(updateActiveState)
                                                 name:NSWindowDidResignMainNotification object:[self window]];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(updateActiveState)
                                                 name:NSWindowDidBecomeKeyNotification object:[self window]];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(updateActiveState)
                                                 name:NSWindowDidResignKeyNotification object:[self window]];


    NSTrackingAreaOptions focusTrackingAreaOptions = NSTrackingActiveInActiveApp;
    focusTrackingAreaOptions |= NSTrackingMouseEnteredAndExited;
    focusTrackingAreaOptions |= NSTrackingAssumeInside;
    focusTrackingAreaOptions |= NSTrackingInVisibleRect;

    NSTrackingArea *focusTrackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                                         options:focusTrackingAreaOptions owner:self userInfo:nil];
    [self addTrackingArea:focusTrackingArea];
    [focusTrackingArea release];
}

- (void)mouseDown:(NSEvent *)theEvent {
    pressedState = YES;
    hoverState = NO;

    if (!activeState) {
        [self.window makeKeyAndOrderFront:self];
        [self.window setOrderedIndex:0];
    }
    [self updateButtonStates];
}


- (void)mouseUp:(NSEvent *)theEvent {
    pressedState = NO;
    hoverState = NO;
    [self updateButtonStates];
    [[self window] toggleFullScreen:self];
}

- (void)mouseEntered:(NSEvent *)theEvent {
    pressedState = NO;
    hoverState = YES;
    [self updateButtonStates];
    [super mouseEntered:theEvent];
}
- (void)mouseExited:(NSEvent *)theEvent {
    pressedState = NO;
    hoverState = NO;
    [self updateButtonStates];
    [super mouseEntered:theEvent];
}

- (void)updateActiveState {
    activeState = [self.window isKeyWindow];
    [self updateButtonStates];
}


-(void)updateButtonStates{
    if (self == nil)
        return;
    if (pressedState) {
        [self setImage:pressed];
    } else if (activeState) {
        if (hoverState) {
            [self setImage:hover];
        } else {
            [self setImage:active];
        }
    } else {
       [self setImage:inactive];
    }
}


- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

@end
