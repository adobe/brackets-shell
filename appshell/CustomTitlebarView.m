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
#import "CustomTitlebarView.h"
#import "client_colors_mac.h"

#define titleTextHeight    13
#define fudge               4

@implementation CustomTitlebarView

@synthesize titleString;

- (void)drawRect:(NSRect)dirtyRect
{
    NSColorSpace    *sRGB = [NSColorSpace sRGBColorSpace];
    NSColor *fillColor = [NSColor colorWithColorSpace:sRGB components:fillComp count:4];
    NSRect windowFrame   = [NSWindow frameRectForContentRect:[[[self window] contentView] bounds] styleMask:[[self window] styleMask]];
    NSRect contentBounds = [[[self window] contentView] bounds];

    NSRect titlebarRect = NSMakeRect(0, 0, self.bounds.size.width, windowFrame.size.height - contentBounds.size.height);
    titlebarRect.origin.y = self.bounds.size.height - titlebarRect.size.height;

    [[NSColor clearColor] set];
    NSRectFill( titlebarRect );

    [fillColor set];

    [NSGraphicsContext saveGraphicsState];
    //This constant matches the radius for other macosx apps.
    //For some reason if we use the default value it is double that of safari etc.
    float cornerRadius = 4.0f;

    // make a clip mask that is rounded on top and square on the bottom...
    NSBezierPath* clipPath = [NSBezierPath bezierPath];
    [clipPath appendBezierPathWithRoundedRect:titlebarRect xRadius:cornerRadius yRadius:cornerRadius];
    [clipPath moveToPoint: NSMakePoint(titlebarRect.origin.x, titlebarRect.origin.y)];
    [clipPath appendBezierPathWithRect: NSMakeRect(titlebarRect.origin.x, titlebarRect.origin.y, titlebarRect.size.width, titlebarRect.size.height / 2)];
    [clipPath addClip];

    // Fill in with the Dark UI  color
    NSRectFill(titlebarRect);


    NSFont          *titleFont = [NSFont titleBarFontOfSize:titleTextHeight];
    CGFloat         stringWidth = [self widthOfString:titleString withFont:titleFont];
    NSColor         *activeColor = [NSColor colorWithColorSpace:sRGB components:activeComp count:4];
    NSColor         *inactiveColor = [NSColor colorWithColorSpace:sRGB components:inactiveComp count:4];

    NSLayoutManager *lm = [[NSLayoutManager alloc] init];
    int             height = [lm defaultLineHeightForFont:titleFont];
    [lm release];

    // Draw the title text
    if (stringWidth)
    {
        NSRect          textRect = NSMakeRect(titlebarRect.origin.x + ((titlebarRect.size.width / 2) - (stringWidth / 2)),
                                              titlebarRect.origin.y + ((titlebarRect.size.height / 2) - (height / 2)) - fudge,
                                              titlebarRect.size.width,
                                              titlebarRect.size.height);

        [titleString drawInRect:textRect withAttributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                         titleFont, NSFontAttributeName,
                                                         [NSApp isActive] ? activeColor : inactiveColor,
                                                         NSForegroundColorAttributeName,
                                                         nil]];
    }

    [NSGraphicsContext restoreGraphicsState];
}

- (CGFloat)widthOfString:(NSString *)string withFont:(NSFont *)font
{
    if (string == nil || [string length] == 0)
        return 0.0f;
    
    NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, nil];
    NSAttributedString *attributedString = [[NSAttributedString alloc] initWithString:string attributes:attributes];
    CGFloat stringWidth = [attributedString size].width;
    [attributedString release];
    return stringWidth;
}

#pragma mark Property accessors

- (NSString *)titleString
{
    return titleString;
}

- (void)setTitleString:(NSString *)aString
{
    if ((!titleString && !aString) || (titleString && aString && [titleString isEqualToString:aString]))
		return;
    titleString = [aString copy];
    [self setNeedsDisplay:YES];
}


@end
