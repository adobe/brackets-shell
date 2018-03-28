
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
+ (BOOL) IsAutoUpdateInProgress;
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
    if(launchPath) {        
        // Create a NSTask for the script
        NSTask* pScriptTask = [[NSTask alloc] init];
        pScriptTask.launchPath = launchPath;
        pScriptTask.arguments = argsArray;
        
        // Launch the script.
        [pScriptTask launch];
        int returnCode = INT_MAX;   //AutoUpdate : TODO
        if(waitUntilExit) {
            [pScriptTask waitUntilExit];
            returnCode = pScriptTask.terminationStatus;
        }
        [pScriptTask release];
        return returnCode;
    }
    //AutoUpdate : TODO : Error handling
    return -1; 
}

// Runs the app update
+ (void) RunAppUpdate{
    
    if([UpdateHelper launchInstaller]) {

        NSString *mountPoint = @"/Volumes/BracketsAutoUpdate";
        NSString *imagePath = _params[@"installerPath"];
        NSString *logFilePath = _params[@"logFilePath"];
        NSString *installDir = _params[@"installDir"];
        NSString *bracketsAppName = _params[@"appName"];
        NSString *updateDir = _params[@"updateDir"];
        
        //Util paths
        NSString* hdiPath = @"/usr/bin/hdiutil";
        NSString* shPath = @"/bin/sh";
        NSString* nohupPath = @"/usr/bin/nohup";
    
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
    
            NSString *scriptPath = [mountPoint stringByAppendingString:@"/update.sh"];
            pArgs = [NSArray arrayWithObjects:shPath, scriptPath, @"-a", bracketsAppName, @"-b", installDir,  @"-l", logFilePath, @"-m", mountPoint,  @"-t", updateDir, @"&", nil];
            retval = RunScript(nohupPath, pArgs, false);
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
        ExtensionString installDir = argsDict->GetString("installDir").ToString();
        ExtensionString appName = argsDict->GetString("appName").ToString();
        ExtensionString updateDir = argsDict->GetString("updateDir").ToString();
        
        [UpdateHelper setLaunchInstaller:true];
        
        NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
        [dict setObject:[NSString stringWithUTF8String:installerPath.c_str()] forKey:@"installerPath"];
        [dict setObject:[NSString stringWithUTF8String:logFilePath.c_str()] forKey:@"logFilePath"];
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

// Checks if the auto update is in progress
int32 IsAutoUpdateInProgress(bool &isAutoUpdateInProgress)
{
    isAutoUpdateInProgress = [UpdateHelper launchInstaller];
    return NO_ERROR;
}

