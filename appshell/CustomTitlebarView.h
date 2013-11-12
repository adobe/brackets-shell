//
//  CustomTitlebarView.h
//  appshell
//
//  Created by Bob Easterday on 11/6/13.
//
//

#import <Cocoa/Cocoa.h>

@interface CustomTitlebarView : NSView
{
    NSString    *titleString;
}

@property (nonatomic, strong) NSString	*titleString;

@end
