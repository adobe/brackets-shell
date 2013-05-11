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

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include "appshell_node_process.h"
#include "appshell_node_process_internal.h"

#include "util.h"
#include "config.h"

// NOTE ON THREAD SAFETY: This code is not thread-safe. All of the methods
// in the NodeWrapper class are inteded to be called *only* by the main
// thread.

// Class that wraps a node process and stores state data
@interface NodeWrapper : NSObject {
    NSTask *task;
    CFAbsoluteTime lastStartTime;
    int state;
}

-(bool) start;
-(void) stop;
-(int) getState;
-(void) setState: (int)newState;
-(void) receiveDataFromTask: (NSNotification *)aNotification;
-(void) sendDataToTask: (NSString *)dataString;

@end

@implementation NodeWrapper

-(id) init {
    if (self = [super init]) {
        task = NULL;
        lastStartTime = NULL;
        state = BRACKETS_NODE_NOT_YET_STARTED;
    }
    return self;
}

-(void)dealloc {
    [self stop]; // will release the task object
    [super dealloc];
}

// Accessor for process state
-(int) getState {
    return state;
}

// Mutator for process state
-(void) setState:(int)newState {
    state = newState;
}

// Starts a new node process and registers appropriate handlers
-(bool) start {
    state = BRACKETS_NODE_PORT_NOT_YET_SET;
    
    lastStartTime = CFAbsoluteTimeGetCurrent();
    
    NSString *appPath = [[NSBundle mainBundle] bundlePath];
    NSString *nodePath = [appPath stringByAppendingString:NODE_EXECUTABLE_PATH];
    NSString *nodeJSPath = [appPath stringByAppendingString:NODE_CORE_PATH];
    
    task = [[NSTask alloc] init];
    [task setStandardOutput: [NSPipe pipe]];
    [task setStandardInput: [NSPipe pipe]];

    // Enable the line below to pipe stderr to stdout. In our case, we don't want this
    // behavior. We already map console.error to a Logger function on the node side. On
    // the off chance that an extension author explicitly writes to stderr, we don't 
    // want the messages polluting our communication channel. We *could* imagine doing
    // something like capturing this and NSLogging it, but that would just encourage this
    // less-than-ideal debugging workflow.
    //
    // If this is not enabled, stderr goes to its own pipe. If that pipe is never opened
    // on the parent side (as is currently the case in this code) the data is thrown away
    //
    // [task setStandardError: [task standardOutput]];  
    
    [task setLaunchPath: nodePath];
    
    NSArray *arguments = [NSArray arrayWithObject:nodeJSPath];
    [task setArguments: arguments];
    
    // Here we register as an observer of the NSFileHandleReadCompletionNotification, which lets
    // us know when there is data waiting for us to grab it in the task's file handle (the pipe
    // to which we connected stdout and stderr above).  -receiveDataFromTask: will be called when there
    // is data waiting.  The reason we need to do this is because if the file handle gets
    // filled up, the task will block waiting to send data and we'll never get anywhere.
    // So we have to keep reading data from the file handle as we go.
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(receiveDataFromTask:)
                                                 name: NSFileHandleReadCompletionNotification 
                                               object: [[task standardOutput] fileHandleForReading]];
    // We tell the file handle to go ahead and read in the background asynchronously, and notify
    // us via the callback registered above when we signed up as an observer.  The file handle will
    // send a NSFileHandleReadCompletionNotification when it has data that is available.
    [[[task standardOutput] fileHandleForReading] readInBackgroundAndNotify];
    
    [task launch];
    
    return true;
    
}

// Stops the currently-running node process, and possibly restarts.
-(void) stop {
    // Assume we've failed. We might restart, but until we do, we don't want to send
    // any messages to the task.
    state = BRACKETS_NODE_FAILED;
    
    NSData *data;
    NSString *dataNSString;
    std::string dataStdString("");
    
    if (task != NULL) {
        // It is important to clean up after ourselves so that we don't leave potentially deallocated
        // objects as observers in the notification center; this can lead to crashes.
        [[NSNotificationCenter defaultCenter] removeObserver:self name:NSFileHandleReadCompletionNotification object: [[task standardOutput] fileHandleForReading]];
        
        // Make sure the task has actually stopped!
        [task terminate];
        
        // Finish reading anything that was put in the pipe prior to termination that we
        // haven't yet read. In theory, there could be a command in the pipe that we need
        // to execute.
        while ((data = [[[task standardOutput] fileHandleForReading] availableData]) && [data length])
        {
            dataNSString = [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
            dataStdString += [dataNSString UTF8String];
        }

        if (dataStdString.length() > 0) {
            processIncomingData(dataStdString);
        }
        
        [task release];
        task = NULL;
    }
    
    CFAbsoluteTime elapsed = CFAbsoluteTimeGetCurrent() - lastStartTime;
    if (elapsed > BRACKETS_NODE_AUTO_RESTART_TIMEOUT) {
        [self start];
    } else {
        NSLog(@"Node process did not stay running long enough. Failing.");
    }
}

// Handler for new data from node process
-(void) receiveDataFromTask: (NSNotification *)aNotification {
    NSData *data = [[aNotification userInfo] objectForKey:NSFileHandleNotificationDataItem];
    // If the length of the data is zero, then the task is basically over - there is nothing
    // more to get from the handle so we may as well shut down.
    if ([data length]) {
        // Send the data on to the controller; we can't just use +stringWithUTF8String: here
        // because -[data bytes] is not necessarily a properly terminated string.
        // -initWithData:encoding: on the other hand checks -[data length]
        
        NSString *dataNSString = [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
        
        std::string dataStdString([dataNSString UTF8String]);
        processIncomingData(dataStdString);

        // we need to schedule the file handle go read more data in the background again.
        [[aNotification object] readInBackgroundAndNotify];
        
    } else {
        // We're finished here
        [self stop];
    }
    
}

// Sends data to the current node process
-(void) sendDataToTask: (NSString *)dataString {    
    if (state > 0) {
        const char * utf8DataString = [dataString UTF8String];
        NSData *data = [[[NSData alloc] initWithBytes:utf8DataString length:strlen(utf8DataString)] autorelease];
        [[[task standardInput] fileHandleForWriting] writeData:data];
    }
}

@end

// Singleton
static NodeWrapper* gNodeWrapper = [[NodeWrapper alloc] init];

// Implementation of platform-specific functions from
// appshell_node_process(_internal).h. Essentially wraps a singleton
// instance of NodeWrapper and calls in to the appropriate methods

void startNodeProcess()
{
    if (getNodeState() == BRACKETS_NODE_NOT_YET_STARTED) {
        [gNodeWrapper start];
    }
}

void sendData(const std::string &data) {
    if (getNodeState() >= 0) {
        NSString *dataString = [[[NSString alloc] initWithUTF8String:data.c_str()] autorelease];
        [gNodeWrapper sendDataToTask:dataString];
    }
}

int getNodeState() {
    return [gNodeWrapper getState];
}

void setNodeState(int state) {
    [gNodeWrapper setState:state];
}
