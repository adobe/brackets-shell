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
#include "native_menu_model.h"
#include "appshell_node_process.h"

namespace appshell_extensions {

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
        if (argList->GetSize() > 0) {
            callbackId = argList->GetInt(0);
            
            if (callbackId != -1)
                responseArgs->SetInt(0, callbackId);
        }
        
        if (message_name == "OpenLiveBrowser") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - argURL
            //  2: bool - enableRemoteDebugging
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
            // Parameters
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                CloseLiveBrowser(browser, response);
            
                // Skip standard callback handling. CloseLiveBrowser fires the
                // callback asynchronously.
                return true;
            }
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
        } else if (message_name == "IsNetworkDrive") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - directory path
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            bool isRemote = false;
            if (error == NO_ERROR) {
                ExtensionString path = argList->GetString(1);                
                error = IsNetworkDrive(path, isRemote);
            }
            
            // Set response args for this function
            responseArgs->SetBool(2, isRemote);
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
        } else if (message_name == "MakeDir") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - directory path
            //  2: number - mode
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_INT) {
                error = ERR_INVALID_PARAMS;
            }
          
            if (error == NO_ERROR) {
                ExtensionString pathname = argList->GetString(1);
                int32 mode = argList->GetInt(2);
              
                error = MakeDir(pathname, mode);
            }
            // No additional response args for this function
        } else if (message_name == "Rename") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - old path
            //  2: string - new path
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
          
            if (error == NO_ERROR) {
                ExtensionString oldName = argList->GetString(1);
                ExtensionString newName = argList->GetString(2);
            
                error = Rename(oldName, newName);
            }
          // No additional response args for this function
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
            
            // The CEF-hosted dev tools do not work. Open in a separate browser window instead.
            // handler->ShowDevTools(browser);
            
            ExtensionString url(browser->GetHost()->GetDevToolsURL(true));
            OpenLiveBrowser(url, false);

        } else if (message_name == "GetNodeState") {
            // Parameters:
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                int32 port = 0;
                int32 portOrErr = getNodeState();
                if (portOrErr < 0) { // there is an error
                    error = portOrErr;
                } else {
                    port = portOrErr;
                    error = NO_ERROR;
                }
                responseArgs->SetInt(2, port);
            }
            
        } else if (message_name == "QuitApplication") {
            // Parameters - none

            // The DispatchCloseToNextBrowser() call initiates a quit sequence. The app will
            // quit if all browser windows are closed.
            handler->DispatchCloseToNextBrowser();

        } else if (message_name == "AbortQuit") {
            // Parameters - none
          
            handler->AbortQuit();
        } else if (message_name == "OpenURLInDefaultBrowser") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - url
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString url = argList->GetString(1);
                
                error = OpenURLInDefaultBrowser(url);
                
                // No additional response args for this function
            }
        } else if (message_name == "ShowOSFolder") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - path
            
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
          
            if (error == NO_ERROR) {
                ExtensionString path = argList->GetString(1);
                error = ShowFolderInOSWindow(path);
            }
        } else if (message_name == "GetPendingFilesToOpen") {
            // Parameters:
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString files;
                error = GetPendingFilesToOpen(files);
                responseArgs->SetString(2, files.c_str());
            }
        } else if (message_name == "AddMenu") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menuTitle to display
            //  2: string - menu/command ID
            //  3: string - position - first, last, before, after
            //  4: string - relativeID - ID of other element relative to which this should be positioned (for position before and after)
            if (argList->GetSize() != 5 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING ||
                argList->GetType(4) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString menuTitle = argList->GetString(1);
                ExtensionString command = CefString(argList->GetString(2));
                ExtensionString position = CefString(argList->GetString(3));
                ExtensionString relativeId = CefString(argList->GetString(4));
                
                error = AddMenu(browser, menuTitle, command, position, relativeId);
                // No additional response args for this function
            }
        } else if (message_name == "AddMenuItem") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - parent menu this is part of
            //  2: string - menuTitle to display
            //  3: string - command ID
            //  4: string - keyboard shortcut
            //  5: string - display string
            //  6: string - position - first, last, before, after
            //  7: string - relativeID - ID of other element relative to which this should be positioned (for position before and after)
            if (argList->GetSize() != 8 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING ||
                argList->GetType(4) != VTYPE_STRING ||
                argList->GetType(5) != VTYPE_STRING ||
                argList->GetType(6) != VTYPE_STRING ||
                argList->GetType(7) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString parentCommand = argList->GetString(1);
                ExtensionString menuTitle = argList->GetString(2);
                ExtensionString command = argList->GetString(3);
                ExtensionString key = argList->GetString(4);
                ExtensionString displayStr = argList->GetString(5);
                ExtensionString position = argList->GetString(6);
                ExtensionString relativeId = argList->GetString(7);

                error = AddMenuItem(browser, parentCommand, menuTitle, command, key, displayStr, position, relativeId);
                // No additional response args for this function
            }
        } else if (message_name == "RemoveMenu") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId;
                commandId = argList->GetString(1);
                error = RemoveMenu(browser, commandId);
            }
            // No response args for this function
        } else if (message_name == "RemoveMenuItem") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId;
                commandId = argList->GetString(1);
                error = RemoveMenuItem(browser, commandId);
            }
            // No response args for this function
        } else if (message_name == "GetMenuItemState") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menu/command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId = CefString(argList->GetString(1));
                ExtensionString menuTitle;
                bool checked, enabled;
                int index;
                error = GetMenuItemState(browser, commandId, enabled, checked, index);
                responseArgs->SetBool(2, enabled);
                responseArgs->SetBool(3, checked);
                responseArgs->SetInt(4, index);
            }
        }  else if(message_name == "SetMenuItemState") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - commandName
            //  2: bool - enabled
            //  3: bool - checked
            if (argList->GetSize() != 4 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_BOOL ||
                argList->GetType(3) != VTYPE_BOOL) {
                error = ERR_INVALID_PARAMS;
            }

            if (error == NO_ERROR) {
                ExtensionString command = argList->GetString(1);
                bool enabled = argList->GetBool(2);
                bool checked = argList->GetBool(3);
                error = NativeMenuModel::getInstance(getMenuParent(browser)).setMenuItemState(command, enabled, checked);
            }
        } else if (message_name == "SetMenuTitle") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menu/command ID
            //  2: string - menuTitle to display
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING) {
                    error = ERR_INVALID_PARAMS;
            }

            if (error == NO_ERROR) {
                ExtensionString command = CefString(argList->GetString(1));
                ExtensionString menuTitle = argList->GetString(2);

                error = SetMenuTitle(browser, command, menuTitle);
                // No additional response args for this function
            }
        }  else if (message_name == "GetMenuTitle") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menu/command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId = CefString(argList->GetString(1));
                ExtensionString menuTitle;
                error = GetMenuTitle(browser, commandId, menuTitle);
                responseArgs->SetString(2, menuTitle);
            }
        } else if (message_name == "SetMenuItemShortcut") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - command ID
            //  2: string - shortcut
            //  3: string - display string
            if (argList->GetSize() != 4 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId = argList->GetString(1);
                ExtensionString shortcut = argList->GetString(2);
                ExtensionString displayStr = argList->GetString(3);
                
                error = SetMenuItemShortcut(browser, commandId, shortcut, displayStr);
                // No additional response args for this function
            }
        } else if (message_name == "GetMenuPosition") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menu/command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId;
                ExtensionString parentId;
                int index;
                commandId = argList->GetString(1);
                error = GetMenuPosition(browser, commandId, parentId, index);
                responseArgs->SetString(2, parentId);
                responseArgs->SetInt(3, index);
            }
        } else if (message_name == "Drag") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menu/command ID
            // if (argList->GetSize() != 2 ||
            //     argList->GetType(1) != VTYPE_STRING) {
            //     error = ERR_INVALID_PARAMS;
            // }
            
            if (error == NO_ERROR) {
                
                Drag(browser);
            }
        } 
        else {
            fprintf(stderr, "Native function not implemented yet: %s\n", message_name.c_str());
            return false;
        }
      
        if (callbackId != -1) {
            responseArgs->SetInt(1, error);
          
            // Send response
            browser->SendProcessMessage(PID_RENDERER, response);
        }
      
        return true;
    }
  
    IMPLEMENT_REFCOUNTING(ProcessMessageDelegate);
};
    
void CreateProcessMessageDelegates(ClientHandler::ProcessMessageDelegateSet& delegates) {
    delegates.insert(new ProcessMessageDelegate);
}

//Replace keyStroke with replaceString 
bool fixupKey(ExtensionString& key, ExtensionString keyStroke, ExtensionString replaceString)
{
	size_t idx = key.find(keyStroke, 0);
	if (idx != ExtensionString::npos) {
		key = key.replace(idx, keyStroke.size(), replaceString);
		return true;
	}
	return false;
}

} // namespace appshell_extensions
