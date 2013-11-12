//
//  CustomTitlebarView.m
//  appshell
//
//  Created by Bob Easterday on 11/6/13.
//
//

#import "CustomTitlebarView.h"
#import "client_colors_mac.h"

#define titleTextHeight    16

@implementation CustomTitlebarView

@synthesize titleString;

- (void)drawRect:(NSRect)dirtyRect
{
    NSColorSpace    *sRGB = [NSColorSpace sRGBColorSpace];
	NSRect windowFrame   = [NSWindow frameRectForContentRect:[[[self window] contentView] bounds] styleMask:[[self window] styleMask]];
	NSRect contentBounds = [[[self window] contentView] bounds];
    
	NSRect titlebarRect = NSMakeRect(0, 0, self.bounds.size.width, windowFrame.size.height - contentBounds.size.height);
	titlebarRect.origin.y = self.bounds.size.height - titlebarRect.size.height;
    
    [[NSColor clearColor] set];
    NSRectFill( titlebarRect );
    
    //This constant matches the radius for other macosx apps.
    //For some reason if we use the default value it is double that of safari etc.
    float cornerRadius = 4.0f;

    [[NSBezierPath bezierPathWithRoundedRect:titlebarRect
                                     xRadius:cornerRadius
                                     yRadius:cornerRadius] addClip];
    [[NSBezierPath bezierPathWithRect:titlebarRect] addClip];
    
    NSColor *fillColor = [NSColor colorWithColorSpace:sRGB components:fillComp count:4];
    [fillColor set];
    NSRectFill(titlebarRect);
    
    NSFont          *titleFont = [NSFont fontWithName:@"HelveticaNeue-Bold" size:titleTextHeight];
    CGFloat         stringWidth = [self widthOfString:titleString withFont:titleFont];
    NSColor         *activeColor = [NSColor colorWithColorSpace:sRGB components:activeComp count:4];
    NSColor         *inactiveColor = [NSColor colorWithColorSpace:sRGB components:inactiveComp count:4];
    
    if (stringWidth)
    {
        NSRect          textRect = NSMakeRect(titlebarRect.origin.x + ((titlebarRect.size.width / 2) - (stringWidth / 2)),
                                              titlebarRect.origin.y + ((titlebarRect.size.height / 2) - (titleTextHeight / 2)),
                                              titlebarRect.size.width,
                                              titlebarRect.size.height);

        [titleString drawInRect:textRect withAttributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                         titleFont, NSFontAttributeName,
                                                         [NSApp isActive] ? activeColor : inactiveColor,
                                                         NSForegroundColorAttributeName,
                                                         nil]];
    }
}

- (CGFloat)widthOfString:(NSString *)string withFont:(NSFont *)font
{
    if (string == nil || [string length] == 0)
        return 0.0f;
    
    NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, nil];
    return [[[NSAttributedString alloc] initWithString:string attributes:attributes] size].width;
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
