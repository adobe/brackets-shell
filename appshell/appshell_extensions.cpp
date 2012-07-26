/*
 * Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
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

#include "appshell_extensions.h"
#include "appshell_extensions_platform.h"

namespace appshell_extensions {

namespace {
        
class ProcessMessageDelegate : public ClientHandler::ProcessMessageDelegate {
public:
    ProcessMessageDelegate()
    {
    }

    // From ClientHandler::ProcessMessageDelegate.
    virtual bool OnProcessMessageReceived(
                      CefRefPtr<ClientHandler> handler,
                      CefRefPtr<CefBrowser> browser,
                      CefProcessId source_process,
                      CefRefPtr<CefProcessMessage> message) OVERRIDE {
        std::string message_name = message->GetName();
        CefRefPtr<CefListValue> argList = message->GetArgumentList();
        int32 callbackId = -1;
        int32 error = NO_ERROR;
        CefRefPtr<CefProcessMessage> response = 
            CefProcessMessage::Create("invokeCallback");
        CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
        
        // V8 extension messages are handled here. These messages come from the 
        // render process thread (in client_app.cpp), and have the following format:
        //   name - the name of the native function to call
        //   argument0 - the id of this message. This id is passed back to the
        //               render process in order to execute callbacks
        //   argument1 - argumentN - the arguments for the function
        //
        // Note: Functions without callback can be specified, but they *cannot*
        // take any arguments.
        
        // If we have any arguments, the first is the callbackId
        if (argList->GetSize() > 0)
            callbackId = argList->GetInt(0);
        
		if (message_name == "OpenLiveBrowser") {
            // Parameters:
            //  0: string - argURL
            //  1: bool - enableRemoteDebugging
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_BOOL) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString argURL = argList->GetString(1);
                bool enableRemoteDebugging = argList->GetBool(2);
				error = OpenLiveBrowser(argURL, enableRemoteDebugging);
			}
		} else if (message_name == "CloseLiveBrowser") {
			//error = CloseLiveBrowser();
		} else if (message_name == "ShowOpenDialog") {
            // Parameters:
            //  0: int32 - callback id
            //  1: bool - allowMultipleSelection
            //  2: bool - chooseDirectory
            //  3: string - title
            //  4: string - initialPath
            //  5: string - fileTypes (space-delimited string)
            if (argList->GetSize() != 6 ||
                argList->GetType(1) != VTYPE_BOOL ||
                argList->GetType(2) != VTYPE_BOOL ||
                argList->GetType(3) != VTYPE_STRING ||
                argList->GetType(4) != VTYPE_STRING ||
                argList->GetType(5) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }

            CefRefPtr<CefListValue> selectedFiles = CefListValue::Create();
           
            if (error == NO_ERROR) {
                bool allowMultipleSelection = argList->GetBool(1);
                bool chooseDirectory = argList->GetBool(2);
                ExtensionString title = argList->GetString(3);
                ExtensionString initialPath = argList->GetString(4);
                ExtensionString fileTypes = argList->GetString(5);
                
                error = ShowOpenDialog(allowMultipleSelection,
                                       chooseDirectory,
                                       title,
                                       initialPath,
                                       fileTypes,
                                       selectedFiles);
            }

            // Set response args for this function
            responseArgs->SetList(2, selectedFiles);
        } else if (message_name == "ReadDir") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - directory path
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            CefRefPtr<CefListValue> directoryContents = CefListValue::Create();
            
            if (error == NO_ERROR) {
                ExtensionString path = argList->GetString(1);
                
                error = ReadDir(path, directoryContents);
            }
            
            // Set response args for this function
            responseArgs->SetList(2, directoryContents);
        } else if (message_name == "GetFileModificationTime") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                uint32 modtime;
                bool isDir;
                
                error = GetFileModificationTime(filename, modtime, isDir);
                
                // Set response args for this function
                responseArgs->SetInt(2, modtime);
                responseArgs->SetBool(3, isDir);
            }
        } else if (message_name == "ReadFile") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            //  2: string - encoding
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                ExtensionString encoding = argList->GetString(2);
                std::string contents = "";
                
                error = ReadFile(filename, encoding, contents);
                
                // Set response args for this function
                responseArgs->SetString(2, contents);
            }
        } else if (message_name == "WriteFile") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            //  2: string - data
            //  3: string - encoding
            if (argList->GetSize() != 4 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                std::string contents = argList->GetString(2);
                ExtensionString encoding = argList->GetString(3);
                
                error = WriteFile(filename, contents, encoding);
                // No additional response args for this function
            }
        } else if (message_name == "SetPosixPermissions") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            //  2: int - mode
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_INT) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                int32 mode = argList->GetInt(2);
                
                error = SetPosixPermissions(filename, mode);
                
                // No additional response args for this function
            }
        } else if (message_name == "DeleteFileOrDirectory") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                
                error = DeleteFileOrDirectory(filename);
                
                // No additional response args for this function
            }
        } else if (message_name == "ShowDeveloperTools") {
            // Parameters - none
            
            handler->ShowDevTools(browser);

        } else if (message_name == "QuitApplication") {
            // Parameters - none
          
            // The DispatchCloseToNextBrowser() call initiates a quit sequence. The app will
            // quit if all browser windows are closed.
            handler->DispatchCloseToNextBrowser();

        } else if (message_name == "AbortQuit") {
            // Parameters - none
          
            handler->AbortQuit();
        } else {
            fprintf(stderr, "Native function not implemented yet: %s\n", message_name.c_str());
            return false;
        }
        
        if (callbackId != -1) {
            // Set common response args (callbackId and error)
            responseArgs->SetInt(0, callbackId);
            responseArgs->SetInt(1, error);
            
            // Send response
            browser->SendProcessMessage(PID_RENDERER, response);
        }
        
        return true;
    }
    
    IMPLEMENT_REFCOUNTING(ProcessMessageDelegate);
};
    
} // namespace
    
void CreateProcessMessageDelegates(
                                   ClientHandler::ProcessMessageDelegateSet& delegates) {
    delegates.insert(new ProcessMessageDelegate);
}
  
} // namespace appshell_extensions
