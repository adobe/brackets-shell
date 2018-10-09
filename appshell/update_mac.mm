
/*
 * Copyright (c) 2018 - present Adobe Systems Incorporated. All rights reserved.
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

// This file contains the core logic of handling auto update on MAC.


#import <Foundation/Foundation.h>
#include "update.h"

//Helper class for app auto update
@interface UpdateHelper : NSObject

+ (BOOL) launchInstaller;
+ (NSMutableDictionary *) params;

+ (void) setLaunchInstaller:(BOOL)shouldlaunchInstaller;
+ (void) setParams:(NSMutableDictionary *)params;

+ (void) RunAppUpdate;
+ (int) getCurrentProcessID;
+ (BOOL) createFileItNotExists:(NSString *)filePath;
@end


@implementation UpdateHelper

static BOOL _launchInstaller;
static NSMutableDictionary * _params;


+ (BOOL) launchInstaller {
    return _launchInstaller;
}

+ (NSMutableDictionary *) params {
    return _params;
}

// Setter for boolean for conditional launch of installer
+ (void) setLaunchInstaller:(BOOL)shouldlaunchInstaller {
    _launchInstaller = shouldlaunchInstaller;
}

// Setter for update parameters
+ (void) setParams:(NSMutableDictionary *)params {
    _params = params;
}


// Runs a script, given the script path, and args array.
int RunScript(NSString* launchPath, NSArray* argsArray, BOOL waitUntilExit)
{
    int returnCode = -1;
    if(launchPath) {        
        // Create a NSTask for the script
        NSTask* pScriptTask = [[NSTask alloc] init];
        pScriptTask.launchPath = launchPath;
        pScriptTask.arguments = argsArray;
        
        // Launch the script.
        [pScriptTask launch];
        if(waitUntilExit) {
            [pScriptTask waitUntilExit];
            returnCode = pScriptTask.terminationStatus;
        }
        [pScriptTask release];
    }
    return returnCode;
}

// Retrieves PID of the currently running instance of Brackets
+ (int) getCurrentProcessID {
    
    return [[NSProcessInfo processInfo] processIdentifier];
    
}

// Creates a file with filePath, if does not exist
+ (BOOL) createFileItNotExists:(NSString *)filePath {
    BOOL returnCode = TRUE;
    if (![[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
        returnCode = [[NSFileManager defaultManager] createFileAtPath:filePath contents:nil attributes:nil];
    }
    return returnCode;
}

// Runs the app update
+ (void) RunAppUpdate{
    
    if([UpdateHelper launchInstaller]) {

        NSString *mountPoint = @"/Volumes/BracketsAutoUpdate";
        
        if (_params || [_params count] != 0) {
            NSString *imagePath = _params[@"installerPath"];
            NSString *logFilePath = _params[@"logFilePath"];
            NSString *installStatusFilePath = _params[@"installStatusFilePath"];
            NSString *installDir = _params[@"installDir"];
            NSString *bracketsAppName = _params[@"appName"];
            NSString *updateDir = _params[@"updateDir"];
            
            //Util paths
            NSString* hdiPath = @"/usr/bin/hdiutil";
            NSString* shPath = @"/bin/sh";
            NSString* nohupPath = @"/usr/bin/nohup";
            
            if ([hdiPath length] == 0 || [shPath length] == 0 || [nohupPath length] == 0 ) {
                BOOL filePresent = [self createFileItNotExists:installStatusFilePath];
                if (filePresent) {
                    NSString *logStr;
                    if([hdiPath length] == 0 ) {
                        logStr = @"ERROR: hdiutil command could not be found: BA_01";
                        [logStr writeToFile:installStatusFilePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
                    }
                    if([shPath length] == 0 ) {
                        logStr = @"ERROR: sh command could not be found: BA_02";
                        [logStr writeToFile:installStatusFilePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
                    }
                    if([nohupPath length] == 0 ) {
                        logStr = @"ERROR: nohupPath command could not be found: BA_03";
                        [logStr writeToFile:installStatusFilePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
                    }
                }

            }
            else {
                // Unmount the already existing disk image, if any
                NSArray* pArgs = [NSArray arrayWithObjects:@"detach", mountPoint,  @"-force", nil];
            
                int retval = RunScript(hdiPath, pArgs, true);
                while (retval != 1) {
                    retval = RunScript(hdiPath, pArgs, true);
                }
            
                // Mount the new disk image
                pArgs = [NSArray arrayWithObjects:@"attach", imagePath, @"-mountpoint", mountPoint, @"-nobrowse", nil];
            
                retval = RunScript(hdiPath, pArgs, true);
            
                // Run the update script present inside the dmg
                if (!retval) {
                
                    NSString *scriptPath = [mountPoint stringByAppendingString:@"/.update.sh"];
                
                    if (![[NSFileManager defaultManager] fileExistsAtPath:scriptPath]) {
                        BOOL filePresent = [self createFileItNotExists:installStatusFilePath];
                        if (filePresent) {
                            NSString *logStr = @"ERROR: update script could not be found: BA_04";
                            [logStr writeToFile:installStatusFilePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
                        }
                    }
                    else {
                        int pid = [self getCurrentProcessID];
                        NSString* pidString = [NSString stringWithFormat:@"%d", pid];
                        BOOL isAdmin = NO;

                        NSString *installDirPath = installDir;
                        installDirPath = [installDirPath stringByAppendingString:@"/"];
                        NSString *bracketsAppPath = installDirPath;
                        bracketsAppPath = [bracketsAppPath stringByAppendingString:bracketsAppName];
                        
                        NSFileManager *fm = [NSFileManager defaultManager];
                        if([fm isWritableFileAtPath:installDirPath] && [fm isWritableFileAtPath:bracketsAppPath] )
                        {
                            isAdmin = YES;
                        }
                        
                        
                        /*Build the Apple Script String*/
                        NSAppleEventDescriptor *returnDescriptor = NULL;
                        NSString * appleScriptString;
                        
                        appleScriptString = [NSString stringWithFormat:@"do shell script \"%@ %@ %@ -a %@ -b '%@' -l '%@' -m %@  -t '%@' -p %@ > /dev/null 2>&1 &\"", nohupPath, shPath, scriptPath, bracketsAppName, installDir, logFilePath, mountPoint, updateDir, pidString];
                        
                        if(!isAdmin) {
                            appleScriptString = [appleScriptString stringByAppendingString:@" with administrator privileges"];
                        }

                        NSAppleScript *theScript = [[NSAppleScript alloc] initWithSource:appleScriptString];
                        
                        NSDictionary *theError = nil;
                        
                        returnDescriptor = [theScript executeAndReturnError: &theError];
                        if(returnDescriptor == NULL) {
                            NSString *logStr = @"ERROR: scriptexecution Failed error: BA_06";
                            [logStr writeToFile:installStatusFilePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
                        }
                    }
                }
            }
        }

    }
}

// Setter for update parametes
+ (void) SetUpdateArgs:(NSMutableDictionary *)dict {
    [self setParams:dict];
}
@end

// Sets the command line arguments to the update script
int32 SetInstallerCommandLineArgs(CefString &updateArgs)
{
    CefRefPtr<CefDictionaryValue> argsDict;
    int32 error =  ParseCommandLineParamsJSON(updateArgs, argsDict);
    if(error == NO_ERROR && argsDict.get())
    {
        ExtensionString installerPath = argsDict->GetString("installerPath").ToString();
        ExtensionString logFilePath = argsDict->GetString("logFilePath").ToString();
        ExtensionString installStatusFilePath = argsDict->GetString("installStatusFilePath").ToString();
        ExtensionString installDir = argsDict->GetString("installDir").ToString();
        ExtensionString appName = argsDict->GetString("appName").ToString();
        ExtensionString updateDir = argsDict->GetString("updateDir").ToString();
        
        [UpdateHelper setLaunchInstaller:true];
        
        NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
        [dict setObject:[NSString stringWithUTF8String:installerPath.c_str()] forKey:@"installerPath"];
        [dict setObject:[NSString stringWithUTF8String:logFilePath.c_str()] forKey:@"logFilePath"];
        [dict setObject:[NSString stringWithUTF8String:installStatusFilePath.c_str()] forKey:@"installStatusFilePath"];
        [dict setObject:[NSString stringWithUTF8String:installDir.c_str()] forKey:@"installDir"];
        [dict setObject:[NSString stringWithUTF8String:appName.c_str()] forKey:@"appName"];
        [dict setObject:[NSString stringWithUTF8String:updateDir.c_str()] forKey:@"updateDir"];

        argsDict = NULL;
        [UpdateHelper SetUpdateArgs:dict];
    }
    return error;
}

// Runs the app auto update
int32 RunAppUpdate(){
    [UpdateHelper RunAppUpdate];
    return NO_ERROR;
}
