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
#import "TrafficLightButton.h"

//these are defined in MainMainu.xib file
static const int CLOSE_BUTTON_TAG = 1000;
static const int MINIMIZE_BUTTON_TAG = 1001;
static const int ZOOM_BUTTON_TAG = 1002;

@implementation TrafficLightButton {
    NSImage *inactive;
    NSImage *active;
    NSImage *hover;
    NSImage *pressed;
    NSImage *dirtyInactive;
    NSImage *dirtyActive;
    NSImage *dirtyHover;
    NSImage *dirtyPressed;
    BOOL activeState;
    BOOL hoverState;
    BOOL pressedState;
    BOOL dirtyState;
    BOOL closeButton;
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
    NSString* buttonName = nil;
    //the numbers come from the XIB file.
    if ([self tag] == CLOSE_BUTTON_TAG) {
        buttonName = @"close";
        closeButton = TRUE;
    } else if ([self tag] == MINIMIZE_BUTTON_TAG) {
        buttonName = @"minimize";
    } else if ([self tag] == ZOOM_BUTTON_TAG){
        buttonName = @"zoom";
    }
    active = [[NSImage imageNamed:[NSString stringWithFormat:@"window-%@-active",buttonName]] retain];
    inactive = [[NSImage imageNamed:[NSString stringWithFormat:@"window-%@-inactive",buttonName]] retain];
    hover = [[NSImage imageNamed:[NSString stringWithFormat:@"window-%@-hover",buttonName]] retain];
    pressed = [[NSImage imageNamed:[NSString stringWithFormat:@"window-%@-pressed",buttonName]] retain];
    if (closeButton) {
        dirtyActive = [[NSImage imageNamed:[NSString stringWithFormat:@"window-%@-dirty-active",buttonName]] retain];
        dirtyInactive = [[NSImage imageNamed:[NSString stringWithFormat:@"window-%@-dirty-inactive",buttonName]] retain];
        dirtyHover = [[NSImage imageNamed:[NSString stringWithFormat:@"window-%@-dirty-hover",buttonName]] retain];
        dirtyPressed = [[NSImage imageNamed:[NSString stringWithFormat:@"window-%@-dirty-pressed",buttonName]] retain];
    }

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
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(hoverIn:)
                                                 name:@"TrafficLightsMouseEnter"
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(hoverOut)
                                                 name:@"TrafficLightsMouseExit"
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(setDocumentEdited)
                                                 name:@"TrafficLightsDirty"
                                               object:nil];
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
    hoverState = YES;

    if (closeButton) {
        [[self window] performClose:nil];
        return;
    }
    if ([self tag] == MINIMIZE_BUTTON_TAG) {
        [[self window] performMiniaturize:nil];
        return;
    }
    if ([self tag] == ZOOM_BUTTON_TAG) {
        [[self window] performZoom:nil];
        return;
    }

    [self updateButtonStates];
    [super mouseUp:theEvent];
}

- (void)updateActiveState {
    activeState = [self.window isKeyWindow];
    [self updateButtonStates];
}

- (void)setDocumentEdited {
    if (closeButton) {
        dirtyState = [[self window] isDocumentEdited];
    }
    [self updateButtonStates];
}

-(void)updateButtonStates{
    if (self == nil)
        return;
    if (pressedState) {
        if (closeButton && dirtyState && dirtyPressed) {
            [self setImage:dirtyPressed];
        } else if (pressed) {
            [self setImage:pressed];
        }
    } else if (activeState) {
        if (hoverState) {
            if (closeButton && dirtyState && dirtyHover) {
                [self setImage:dirtyHover];
            } else if (hover) {
                [self setImage:hover];
            }
        } else {
            if (closeButton && dirtyState && dirtyActive) {
                [self setImage:dirtyActive];
            } else if (active) {
                [self setImage:active];
            }
        }
    } else {
        if (closeButton && dirtyState && dirtyInactive) {
            [self setImage:dirtyInactive];
        } else if (inactive) {
            [self setImage:inactive];
        }
    }
}

- (void)hoverIn:(NSNotification *) notification {

    if ([[notification object] isEqual:[self superview]]) {
        hoverState = YES;
        if (closeButton && dirtyState && dirtyHover) {
            [self setImage:dirtyHover];
        } else if (hover) {
            [self setImage:hover];
        }
    }
}

- (void)hoverOut {
    hoverState = NO;
    if (activeState) {
        if (closeButton && dirtyState && dirtyActive) {
            [self setImage:dirtyActive];
        } else if (active) {
            [self setImage:active];
        }
    } else {
        if (closeButton && dirtyState && dirtyInactive) {
            [self setImage:dirtyInactive];
        } else if (inactive) {
            [self setImage:inactive];
        }
    }
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

@end
