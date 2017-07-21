// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "appshell/browser/client_handler.h"

#include <stdio.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include "include/base/cef_bind.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_parser.h"
#include "include/wrapper/cef_closure_task.h"
#include "appshell/browser/main_context.h"
#include "appshell/browser/resource_util.h"
#include "appshell/browser/root_window_manager.h"
#include "appshell/common/client_switches.h"

#include "appshell/appshell_extensions.h"

#include "appshell/appshell_extensions_platform.h"
#include "appshell/native_menu_model.h"
#include "appshell/appshell_node_process.h"
#include "appshell/config.h"

#include <algorithm>

extern std::vector<CefString> gDroppedFiles;

namespace client {

#if defined(OS_WIN)
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

namespace {

// Custom menu command Ids.
enum client_menu_ids {
  CLIENT_ID_SHOW_DEVTOOLS   = MENU_ID_USER_FIRST,
  CLIENT_ID_CLOSE_DEVTOOLS,
  CLIENT_ID_INSPECT_ELEMENT,
  CLIENT_ID_TESTMENU_SUBMENU,
  CLIENT_ID_TESTMENU_CHECKITEM,
  CLIENT_ID_TESTMENU_RADIOITEM1,
  CLIENT_ID_TESTMENU_RADIOITEM2,
  CLIENT_ID_TESTMENU_RADIOITEM3,
};

// Musr match the value in client_renderer.cc.
const char kFocusedNodeChangedMessage[] = "ClientRenderer.FocusedNodeChanged";

std::string GetTimeString(const CefTime& value) {
  if (value.GetTimeT() == 0)
    return "Unspecified";

  static const char* kMonths[] = {
    "January", "February", "March", "April", "May", "June", "July", "August",
    "September", "October", "November", "December"
  };
  std::string month;
  if (value.month >= 1 && value.month <= 12)
    month = kMonths[value.month - 1];
  else
    month = "Invalid";

  std::stringstream ss;
  ss << month << " " << value.day_of_month << ", " << value.year << " " <<
      std::setfill('0') << std::setw(2) << value.hour << ":" <<
      std::setfill('0') << std::setw(2) << value.minute << ":" <<
      std::setfill('0') << std::setw(2) << value.second;
  return ss.str();
}

std::string GetBinaryString(CefRefPtr<CefBinaryValue> value) {
  if (!value.get())
    return "&nbsp;";

  // Retrieve the value.
  const size_t size = value->GetSize();
  std::string src;
  src.resize(size);
  value->GetData(const_cast<char*>(src.data()), size, 0);

  // Encode the value.
  return CefBase64Encode(src.data(), src.size());
}

std::string GetCertStatusString(cef_cert_status_t status) {
  #define FLAG(flag) if (status & flag) result += std::string(#flag) + "<br/>"
  std::string result;

  FLAG(CERT_STATUS_COMMON_NAME_INVALID);
  FLAG(CERT_STATUS_DATE_INVALID);
  FLAG(CERT_STATUS_AUTHORITY_INVALID);
  FLAG(CERT_STATUS_NO_REVOCATION_MECHANISM);
  FLAG(CERT_STATUS_UNABLE_TO_CHECK_REVOCATION);
  FLAG(CERT_STATUS_REVOKED);
  FLAG(CERT_STATUS_INVALID);
  FLAG(CERT_STATUS_WEAK_SIGNATURE_ALGORITHM);
  FLAG(CERT_STATUS_NON_UNIQUE_NAME);
  FLAG(CERT_STATUS_WEAK_KEY);
  FLAG(CERT_STATUS_PINNED_KEY_MISSING);
  FLAG(CERT_STATUS_NAME_CONSTRAINT_VIOLATION);
  FLAG(CERT_STATUS_VALIDITY_TOO_LONG);
  FLAG(CERT_STATUS_IS_EV);
  FLAG(CERT_STATUS_REV_CHECKING_ENABLED);
  FLAG(CERT_STATUS_SHA1_SIGNATURE_PRESENT);
  FLAG(CERT_STATUS_CT_COMPLIANCE_FAILED);

  if (result.empty())
    return "&nbsp;";
  return result;
}

// Load a data: URI containing the error message.
void LoadErrorPage(CefRefPtr<CefFrame> frame,
                   const std::string& failed_url,
                   cef_errorcode_t error_code,
                   const std::string& other_info) {
  // std::stringstream ss;
  // ss << "<html><head><title>Page failed to load</title></head>"
  //       "<body bgcolor=\"white\">"
  //       "<h3>Page failed to load.</h3>"
  //       "URL: <a href=\"" << failed_url << "\">"<< failed_url << "</a>"
  //       "<br/>Error: " << test_runner::GetErrorString(error_code) <<
  //       " (" << error_code << ")";

  // if (!other_info.empty())
  //   ss << "<br/>" << other_info;

  // ss << "</body></html>";
  // frame->LoadURL(test_runner::GetDataURI(ss.str(), "text/html"));
}

}  // namespace

ClientHandler::ClientHandler(Delegate* delegate,
                             bool is_osr,
                             const std::string& startup_url)
  : is_osr_(is_osr),
    startup_url_(startup_url),
    delegate_(delegate),
    browser_count_(0),
    console_log_file_(MainContext::Get()->GetConsoleLogPath()),
    first_console_message_(true),
    focus_on_editable_field_(false) {
  DCHECK(!console_log_file_.empty());

#if defined(OS_LINUX)
  // Provide the GTK-based dialog implementation on Linux.
  dialog_handler_ = new ClientDialogHandlerGtk();
#endif

  resource_manager_ = new CefResourceManager();

  // Read command line settings.
  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();
  mouse_cursor_change_disabled_ =
      command_line->HasSwitch(switches::kMouseCursorChangeDisabled);
}

void ClientHandler::DetachDelegate() {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(base::Bind(&ClientHandler::DetachDelegate, this));
    return;
  }

  DCHECK(delegate_);
  delegate_ = NULL;
}

/*
bool ClientHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  CEF_REQUIRE_UI_THREAD();

  if (message_router_->OnProcessMessageReceived(browser, source_process,
                                                message)) {
    return true;
  }

  // Check for messages from the client renderer.
  std::string message_name = message->GetName();
  if (message_name == kFocusedNodeChangedMessage) {
    // A message is sent from ClientRenderDelegate to tell us whether the
    // currently focused DOM node is editable. Use of |focus_on_editable_field_|
    // is redundant with CefKeyEvent.focus_on_editable_field in OnPreKeyEvent
    // but is useful for demonstration purposes.
    focus_on_editable_field_ = message->GetArgumentList()->GetBool(0);
    return true;
  }

  return false;
}
*/


bool ClientHandler::OnProcessMessageReceived(
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
        static int call_count = 0;
        call_count++;
        printf("\nCall Count: %d, Method_Name: %s",call_count,  message_name.c_str());

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
           
            if (error == NO_ERROR) {
                bool allowMultipleSelection = argList->GetBool(1);
                bool chooseDirectory = argList->GetBool(2);
                ExtensionString title = argList->GetString(3);
                ExtensionString initialPath = argList->GetString(4);
                ExtensionString fileTypes = argList->GetString(5);
                
#ifdef OS_MACOSX
                ShowOpenDialog(allowMultipleSelection,
                               chooseDirectory,
                               title,
                               initialPath,
                               fileTypes,
                               browser,
                               response);
                
                // Skip standard callback handling. ShowOpenDialog fires the
                // callback asynchronously.
                
                return true;
#else
                CefRefPtr<CefListValue> selectedFiles = CefListValue::Create();
                error = ShowOpenDialog(allowMultipleSelection,
                                       chooseDirectory,
                                       title,
                                       initialPath,
                                       fileTypes,
                                       selectedFiles);
                // Set response args for this function
                responseArgs->SetList(2, selectedFiles);
#endif
                
            }
            
        } else if (message_name == "ShowSaveDialog") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - title
            //  2: string - initialPath
            //  3: string - poposedNewFilename
            if (argList->GetSize() != 4 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }

            if (error == NO_ERROR) {
                ExtensionString title = argList->GetString(1);
                ExtensionString initialPath = argList->GetString(2);
                ExtensionString proposedNewFilename = argList->GetString(3);
                
#ifdef OS_MACOSX
                // Skip standard callback handling. ShowSaveDialog fires the
                // callback asynchronously.
                ShowSaveDialog(title,
                               initialPath,
                               proposedNewFilename,
                               browser,
                               response);
                return true;
#else
                ExtensionString newFilePath;
                error = ShowSaveDialog(title,
                                       initialPath,
                                       proposedNewFilename,
                                       newFilePath);
                
                // Set response args for this function
                responseArgs->SetString(2, newFilePath);
#endif
            }

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
            CefDoMessageLoopWork();
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
        } else if (message_name == "GetFileInfo") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                ExtensionString realPath;
                uint32 modtime;
                double size;
                bool isDir;
                
                error = GetFileInfo(filename, modtime, isDir, size, realPath);
                
                // Set response args for this function
                responseArgs->SetInt(2, modtime);
                responseArgs->SetBool(3, isDir);
                responseArgs->SetInt(4, size);
                responseArgs->SetString(5, realPath);
                printf("...Done!");
            }
            CefDoMessageLoopWork();
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
                bool preserveBOM = false;
                
                error = ReadFile(filename, encoding, contents, preserveBOM);
                
                // Set response args for this function
                responseArgs->SetString(2, contents);
                responseArgs->SetString(3, encoding);
                responseArgs->SetBool(4, preserveBOM);
            }
            CefDoMessageLoopWork();
        } else if (message_name == "WriteFile") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            //  2: string - data
            //  3: string - encoding
            if (argList->GetSize() != 5 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING ||
                argList->GetType(4) != VTYPE_BOOL) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                std::string contents = argList->GetString(2);
                ExtensionString encoding = argList->GetString(3);
                bool preserveBOM = argList->GetBool(4);
                
                error = WriteFile(filename, contents, encoding, preserveBOM);
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
        } else if (message_name == "MoveFileOrDirectoryToTrash") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - path
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString path = argList->GetString(1);
                
                MoveFileOrDirectoryToTrash(path, browser, response);
                
                // Skip standard callback handling. MoveFileOrDirectoryToTrash fires the
                // callback asynchronously.
                return true;
            }
        } else if (message_name == "ShowDeveloperTools") {
            // Parameters - none
            CefWindowInfo wi;
            CefBrowserSettings settings;

#if defined(OS_WIN)
            wi.SetAsPopup(NULL, "DevTools");
#endif
            //browser->GetHost()->ShowDevTools(wi, browser->GetHost()->GetClient(), settings, CefPoint());
            ShowDevTools(browser, CefPoint());

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
            //handler->DispatchCloseToNextBrowser();

        } else if (message_name == "AbortQuit") {
            // Parameters - none
          
            //handler->AbortQuit();
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
        } else if (message_name == "CopyFile") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            //  2: string - dest filename
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString src = argList->GetString(1);
                ExtensionString dest = argList->GetString(2);
                
                error = CopyFile(src, dest);
                // No additional response args for this function
            }
        } else if (message_name == "GetDroppedFiles") {
            // Parameters:
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                std::wstring files;
                
                files = L"[";
                for (unsigned int i = 0; i < gDroppedFiles.size(); i++) {
                    std::wstring file(gDroppedFiles[i]);
                    // Convert windows paths to unix paths
                    replace(file.begin(), file.end(), '\\', '/');
                    files += L"\"";
                    files += file;
                    files += L"\"";
                    if (i < gDroppedFiles.size() - 1) {
                        files += L", ";
                    }
                }
                files += L"]";
                gDroppedFiles.clear();
                
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
                if (error == NO_ERROR) {
                    error = SetMenuItemState(browser, command, enabled, checked);
                }
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
        } else if (message_name == "DragWindow") {     
            // Parameters: none       
            DragWindow(browser);
        } else if (message_name == "GetZoomLevel") {
            // Parameters:
            //  0: int32 - callback id
            double zoomLevel = browser->GetHost()->GetZoomLevel();

            responseArgs->SetDouble(2, zoomLevel);
        } else if (message_name == "SetZoomLevel") {
            // Parameters:
            //  0: int32 - callback id
            //  1: int32 - zoom level

            if (argList->GetSize() != 2 ||
                !(argList->GetType(1) == VTYPE_INT || argList->GetType(1) == VTYPE_DOUBLE)) {
                error = ERR_INVALID_PARAMS;
            }

            if (error == NO_ERROR) {
                // cast to double
                double zoomLevel = 1;
                
                if (argList->GetType(1) == VTYPE_DOUBLE) {
                    zoomLevel = argList->GetDouble(1);
                } else {
                    zoomLevel = argList->GetInt(1);
                }

                browser->GetHost()->SetZoomLevel(zoomLevel);
            }
        }
        else if (message_name == "InstallCommandLineTools") {
            // Parameters:
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                error = InstallCommandLineTools();
            }

		} else if (message_name == "GetMachineHash") {
			// Parameters:
			//  0: int32 - callback id

			responseArgs->SetString(2, GetSystemUniqueID());
		} else if (message_name == "ReadDirWithStats") {
            // Parameters:
            //  0: int32 - callback id
            
            CefRefPtr<CefListValue> uberDict    = CefListValue::Create();
            CefRefPtr<CefListValue> dirContents = CefListValue::Create();
            CefRefPtr<CefListValue> allStats = CefListValue::Create();
            
            ExtensionString path = argList->GetString(1); //"/Users/prashant/brackets-source/brackets/tools/";
            ReadDir(path, dirContents);
            
            // Now we iterator through the contents of directoryContents.
            size_t theSize = dirContents->GetSize();
            for ( size_t iFileEntry = 0; iFileEntry < theSize ; ++iFileEntry) {
                CefRefPtr<CefListValue> fileStats = CefListValue::Create();
                
                ExtensionString theFile  = path + "/";
                ExtensionString fileName = dirContents->GetString(iFileEntry);
                theFile = theFile + fileName;
                
                ExtensionString realPath;
                uint32 modtime;
                double size;
                bool isDir;
                GetFileInfo(theFile, modtime, isDir, size, realPath);
                
                fileStats->SetInt(0, modtime);
                fileStats->SetBool(1, isDir);
                fileStats->SetInt(2, size);
                fileStats->SetString(3, realPath);
                
                allStats->SetList(iFileEntry, fileStats);
                
            }
            
            uberDict->SetList(0, dirContents);
            uberDict->SetList(1, allStats);
            responseArgs->SetList(2, uberDict);

            printf("...Done!");
            
            // Test code working fine.
            /*
            //CefRefPtr<CefDictionaryValue> someDictionary = CefDictionaryValue::Create();
            CefRefPtr<CefListValue> someDictionary = CefListValue::Create();
            CefString theKey   = "key1";
            CefString theValue = "val1";
            someDictionary->SetString(0, theValue);
            theKey = "key2";
            someDictionary->SetInt(1, 2);
            
            CefRefPtr<CefListValue> innerDict = CefListValue::Create();
            innerDict->SetString(0, theValue);
            innerDict->SetInt(1, 2);
            
            
            someDictionary->SetList(2, innerDict);
            
            //responseArgs->SetDictionary(2, someDictionary);
            responseArgs->SetList(2, someDictionary);
             */
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


void ClientHandler::OnBeforeContextMenu(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params,
    CefRefPtr<CefMenuModel> model) {
  CEF_REQUIRE_UI_THREAD();

  if ((params->GetTypeFlags() & (CM_TYPEFLAG_PAGE | CM_TYPEFLAG_FRAME)) != 0) {
    // Add a separator if the menu already has items.
    if (model->GetCount() > 0)
      model->AddSeparator();

    // Add DevTools items to all context menus.
    model->AddItem(CLIENT_ID_SHOW_DEVTOOLS, "&Show DevTools");
    model->AddItem(CLIENT_ID_CLOSE_DEVTOOLS, "Close DevTools");
    model->AddSeparator();
    model->AddItem(CLIENT_ID_INSPECT_ELEMENT, "Inspect Element");

    // Test context menu features.
    BuildTestMenu(model);
  }
}

bool ClientHandler::OnContextMenuCommand(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params,
    int command_id,
    EventFlags event_flags) {
  CEF_REQUIRE_UI_THREAD();

  switch (command_id) {
    case CLIENT_ID_SHOW_DEVTOOLS:
      ShowDevTools(browser, CefPoint());
      return true;
    case CLIENT_ID_CLOSE_DEVTOOLS:
      CloseDevTools(browser);
      return true;
    case CLIENT_ID_INSPECT_ELEMENT:
      ShowDevTools(browser, CefPoint(params->GetXCoord(), params->GetYCoord()));
      return true;
    default:  // Allow default handling, if any.
      return ExecuteTestMenu(command_id);
  }
}

void ClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  CEF_REQUIRE_UI_THREAD();

  // Only update the address for the main (top-level) frame.
  if (frame->IsMain())
    NotifyAddress(url);
}

void ClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  CEF_REQUIRE_UI_THREAD();

  NotifyTitle(title);
}

void ClientHandler::OnFullscreenModeChange(CefRefPtr<CefBrowser> browser,
                                           bool fullscreen) {
  CEF_REQUIRE_UI_THREAD();

  NotifyFullscreen(fullscreen);
}

bool ClientHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                     const CefString& message,
                                     const CefString& source,
                                     int line) {
  CEF_REQUIRE_UI_THREAD();

  // FILE* file = fopen(console_log_file_.c_str(), "a");
  // if (file) {
  //   std::stringstream ss;
  //   ss << "Message: " << message.ToString() << NEWLINE <<
  //         "Source: " << source.ToString() << NEWLINE <<
  //         "Line: " << line << NEWLINE <<
  //         "-----------------------" << NEWLINE;
  //   fputs(ss.str().c_str(), file);
  //   fclose(file);

  //   if (first_console_message_) {
  //     test_runner::Alert(
  //         browser, "Console messages written to \"" + console_log_file_ + "\"");
  //     first_console_message_ = false;
  //   }
  // }

  return false;
}

void ClientHandler::OnBeforeDownload(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    const CefString& suggested_name,
    CefRefPtr<CefBeforeDownloadCallback> callback) {
  CEF_REQUIRE_UI_THREAD();

  // Continue the download and show the "Save As" dialog.
  callback->Continue(MainContext::Get()->GetDownloadPath(suggested_name), true);
}

void ClientHandler::OnDownloadUpdated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    CefRefPtr<CefDownloadItemCallback> callback) {
  CEF_REQUIRE_UI_THREAD();

  if (download_item->IsComplete()) {
    // test_runner::Alert(
    //     browser,
    //     "File \"" + download_item->GetFullPath().ToString() +
    //     "\" downloaded successfully.");
  }
}

bool ClientHandler::OnDragEnter(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                CefDragHandler::DragOperationsMask mask) {
  CEF_REQUIRE_UI_THREAD();

  // Forbid dragging of link URLs.
  if (mask & DRAG_OPERATION_LINK)
    return true;

  return false;
}

void ClientHandler::OnDraggableRegionsChanged(
    CefRefPtr<CefBrowser> browser,
    const std::vector<CefDraggableRegion>& regions) {
  CEF_REQUIRE_UI_THREAD();

  NotifyDraggableRegions(regions);
}

bool ClientHandler::OnRequestGeolocationPermission(
      CefRefPtr<CefBrowser> browser,
      const CefString& requesting_url,
      int request_id,
      CefRefPtr<CefGeolocationCallback> callback) {
  CEF_REQUIRE_UI_THREAD();

  // Allow geolocation access from all websites.
  callback->Continue(true);
  return true;
}

bool ClientHandler::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                                  const CefKeyEvent& event,
                                  CefEventHandle os_event,
                                  bool* is_keyboard_shortcut) {
  CEF_REQUIRE_UI_THREAD();

  if (!event.focus_on_editable_field && event.windows_key_code == 0x20) {
    // Special handling for the space character when an input element does not
    // have focus. Handling the event in OnPreKeyEvent() keeps the event from
    // being processed in the renderer. If we instead handled the event in the
    // OnKeyEvent() method the space key would cause the window to scroll in
    // addition to showing the alert box.
    // if (event.type == KEYEVENT_RAWKEYDOWN)
    //   test_runner::Alert(browser, "You pressed the space bar!");
    return true;
  }

  return false;
}

bool ClientHandler::OnBeforePopup(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& target_url,
    const CefString& target_frame_name,
    CefLifeSpanHandler::WindowOpenDisposition target_disposition,
    bool user_gesture,
    const CefPopupFeatures& popupFeatures,
    CefWindowInfo& windowInfo,
    CefRefPtr<CefClient>& client,
    CefBrowserSettings& settings,
    bool* no_javascript_access) {
  CEF_REQUIRE_IO_THREAD();

  // Return true to cancel the popup window.
  return !CreatePopupWindow(browser, false, popupFeatures, windowInfo, client,
                            settings);
}

void ClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  browser_count_++;

  if (!message_router_) {
    // Create the browser-side router for query handling.
    CefMessageRouterConfig config;
    message_router_ = CefMessageRouterBrowserSide::Create(config);

    // Register handlers with the router.
    MessageHandlerSet::const_iterator it = message_handler_set_.begin();
    for (; it != message_handler_set_.end(); ++it)
      message_router_->AddHandler(*(it), false);
  }

  // Disable mouse cursor change if requested via the command-line flag.
  if (mouse_cursor_change_disabled_)
    browser->GetHost()->SetMouseCursorChangeDisabled(true);

  NotifyBrowserCreated(browser);
}

bool ClientHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  NotifyBrowserClosing(browser);

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void ClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  if (--browser_count_ == 0) {
    // Remove and delete message router handlers.
    MessageHandlerSet::const_iterator it =
        message_handler_set_.begin();
    for (; it != message_handler_set_.end(); ++it) {
      message_router_->RemoveHandler(*(it));
      delete *(it);
    }
    message_handler_set_.clear();
    message_router_ = NULL;
  }

  NotifyBrowserClosed(browser);
}

void ClientHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward) {
  CEF_REQUIRE_UI_THREAD();

  NotifyLoadingState(isLoading, canGoBack, canGoForward);
}

void ClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
  CEF_REQUIRE_UI_THREAD();

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Don't display an error for external protocols that we allow the OS to
  // handle. See OnProtocolExecution().
  if (errorCode == ERR_UNKNOWN_URL_SCHEME) {
    std::string urlStr = frame->GetURL();
    if (urlStr.find("spotify:") == 0)
      return;
  }

  // Load the error page.
  LoadErrorPage(frame, failedUrl, errorCode, errorText);
}

bool ClientHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefRequest> request,
                                   bool is_redirect) {
  CEF_REQUIRE_UI_THREAD();

  message_router_->OnBeforeBrowse(browser, frame);
  return false;
}

bool ClientHandler::OnOpenURLFromTab(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& target_url,
    CefRequestHandler::WindowOpenDisposition target_disposition,
    bool user_gesture) {
  if (target_disposition == WOD_NEW_BACKGROUND_TAB ||
      target_disposition == WOD_NEW_FOREGROUND_TAB) {
    // Handle middle-click and ctrl + left-click by opening the URL in a new
    // browser window.
    MainContext::Get()->GetRootWindowManager()->CreateRootWindow(
        true, is_osr(), CefRect(), target_url);
    return true;
  }

  // Open the URL in the current browser window.
  return false;
}

cef_return_value_t ClientHandler::OnBeforeResourceLoad(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request,
      CefRefPtr<CefRequestCallback> callback) {
  CEF_REQUIRE_IO_THREAD();

  return resource_manager_->OnBeforeResourceLoad(browser, frame, request,
                                                 callback);
}

CefRefPtr<CefResourceHandler> ClientHandler::GetResourceHandler(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request) {
  CEF_REQUIRE_IO_THREAD();

  return resource_manager_->GetResourceHandler(browser, frame, request);
}

CefRefPtr<CefResponseFilter> ClientHandler::GetResourceResponseFilter(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefResponse> response) {
  CEF_REQUIRE_IO_THREAD();

  // return test_runner::GetResourceResponseFilter(browser, frame, request,
  //                                               response);
  return NULL;
}

bool ClientHandler::OnQuotaRequest(CefRefPtr<CefBrowser> browser,
                                   const CefString& origin_url,
                                   int64 new_size,
                                   CefRefPtr<CefRequestCallback> callback) {
  CEF_REQUIRE_IO_THREAD();

  static const int64 max_size = 1024 * 1024 * 20;  // 20mb.

  // Grant the quota request if the size is reasonable.
  callback->Continue(new_size <= max_size);
  return true;
}

void ClientHandler::OnProtocolExecution(CefRefPtr<CefBrowser> browser,
                                        const CefString& url,
                                        bool& allow_os_execution) {
  CEF_REQUIRE_UI_THREAD();

  std::string urlStr = url;

  // Allow OS execution of Spotify URIs.
  if (urlStr.find("spotify:") == 0)
    allow_os_execution = true;
}

bool ClientHandler::OnCertificateError(
    CefRefPtr<CefBrowser> browser,
    ErrorCode cert_error,
    const CefString& request_url,
    CefRefPtr<CefSSLInfo> ssl_info,
    CefRefPtr<CefRequestCallback> callback) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefSSLCertPrincipal> subject = ssl_info->GetSubject();
  CefRefPtr<CefSSLCertPrincipal> issuer = ssl_info->GetIssuer();

  // Build a table showing certificate information. Various types of invalid
  // certificates can be tested using https://badssl.com/.
  std::stringstream ss;
  ss << "X.509 Certificate Information:"
        "<table border=1><tr><th>Field</th><th>Value</th></tr>" <<
        "<tr><td>Subject</td><td>" <<
            (subject.get() ? subject->GetDisplayName().ToString() : "&nbsp;") <<
            "</td></tr>"
        "<tr><td>Issuer</td><td>" <<
            (issuer.get() ? issuer->GetDisplayName().ToString() : "&nbsp;") <<
            "</td></tr>"
        "<tr><td>Serial #*</td><td>" <<
            GetBinaryString(ssl_info->GetSerialNumber()) << "</td></tr>"
        "<tr><td>Status</td><td>" <<
            GetCertStatusString(ssl_info->GetCertStatus()) << "</td></tr>"
        "<tr><td>Valid Start</td><td>" <<
            GetTimeString(ssl_info->GetValidStart()) << "</td></tr>"
        "<tr><td>Valid Expiry</td><td>" <<
            GetTimeString(ssl_info->GetValidExpiry()) << "</td></tr>";

  CefSSLInfo::IssuerChainBinaryList der_chain_list;
  CefSSLInfo::IssuerChainBinaryList pem_chain_list;
  ssl_info->GetDEREncodedIssuerChain(der_chain_list);
  ssl_info->GetPEMEncodedIssuerChain(pem_chain_list);
  DCHECK_EQ(der_chain_list.size(), pem_chain_list.size());

  der_chain_list.insert(der_chain_list.begin(), ssl_info->GetDEREncoded());
  pem_chain_list.insert(pem_chain_list.begin(), ssl_info->GetPEMEncoded());

  for (size_t i = 0U; i < der_chain_list.size(); ++i) {
    ss << "<tr><td>DER Encoded*</td>"
          "<td style=\"max-width:800px;overflow:scroll;\">" <<
              GetBinaryString(der_chain_list[i]) << "</td></tr>"
          "<tr><td>PEM Encoded*</td>"
          "<td style=\"max-width:800px;overflow:scroll;\">" <<
              GetBinaryString(pem_chain_list[i]) << "</td></tr>";
  }

  ss << "</table> * Displayed value is base64 encoded.";

  // Load the error page.
  LoadErrorPage(browser->GetMainFrame(), request_url, cert_error, ss.str());

  return false;  // Cancel the request.
}

void ClientHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                              TerminationStatus status) {
  CEF_REQUIRE_UI_THREAD();

  message_router_->OnRenderProcessTerminated(browser);

  // Don't reload if there's no start URL, or if the crash URL was specified.
  if (startup_url_.empty() || startup_url_ == "chrome://crash")
    return;

  CefRefPtr<CefFrame> frame = browser->GetMainFrame();
  std::string url = frame->GetURL();

  // Don't reload if the termination occurred before any URL had successfully
  // loaded.
  if (url.empty())
    return;

  std::string start_url = startup_url_;

  // Convert URLs to lowercase for easier comparison.
  std::transform(url.begin(), url.end(), url.begin(), tolower);
  std::transform(start_url.begin(), start_url.end(), start_url.begin(),
                 tolower);

  // Don't reload the URL that just resulted in termination.
  if (url.find(start_url) == 0)
    return;

  frame->LoadURL(startup_url_);
}

int ClientHandler::GetBrowserCount() const {
  CEF_REQUIRE_UI_THREAD();
  return browser_count_;
}

void ClientHandler::ShowDevTools(CefRefPtr<CefBrowser> browser,
                                 const CefPoint& inspect_element_at) {
  CefWindowInfo windowInfo;
  CefRefPtr<CefClient> client;
  CefBrowserSettings settings;

  if (CreatePopupWindow(browser, true, CefPopupFeatures(), windowInfo, client,
                        settings)) {
    browser->GetHost()->ShowDevTools(windowInfo, client, settings,
                                     inspect_element_at);
  }
}

void ClientHandler::CloseDevTools(CefRefPtr<CefBrowser> browser) {
  browser->GetHost()->CloseDevTools();
}


bool ClientHandler::CreatePopupWindow(
    CefRefPtr<CefBrowser> browser,
    bool is_devtools,
    const CefPopupFeatures& popupFeatures,
    CefWindowInfo& windowInfo,
    CefRefPtr<CefClient>& client,
    CefBrowserSettings& settings) {
  // Note: This method will be called on multiple threads.

  // The popup browser will be parented to a new native window.
  // Don't show URL bar and navigation buttons on DevTools windows.
  MainContext::Get()->GetRootWindowManager()->CreateRootWindowAsPopup(
      !is_devtools, is_osr(), popupFeatures, windowInfo, client, settings);

  return true;
}


void ClientHandler::NotifyBrowserCreated(CefRefPtr<CefBrowser> browser) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&ClientHandler::NotifyBrowserCreated, this, browser));
    return;
  }

  if (delegate_)
    delegate_->OnBrowserCreated(browser);
}

void ClientHandler::NotifyBrowserClosing(CefRefPtr<CefBrowser> browser) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&ClientHandler::NotifyBrowserClosing, this, browser));
    return;
  }

  if (delegate_)
    delegate_->OnBrowserClosing(browser);
}

void ClientHandler::NotifyBrowserClosed(CefRefPtr<CefBrowser> browser) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&ClientHandler::NotifyBrowserClosed, this, browser));
    return;
  }

  if (delegate_)
    delegate_->OnBrowserClosed(browser);
}

void ClientHandler::NotifyAddress(const CefString& url) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&ClientHandler::NotifyAddress, this, url));
    return;
  }

  if (delegate_)
    delegate_->OnSetAddress(url);
}

void ClientHandler::NotifyTitle(const CefString& title) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&ClientHandler::NotifyTitle, this, title));
    return;
  }

  if (delegate_)
    delegate_->OnSetTitle(title);
}

void ClientHandler::NotifyFullscreen(bool fullscreen) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&ClientHandler::NotifyFullscreen, this, fullscreen));
    return;
  }

  if (delegate_)
    delegate_->OnSetFullscreen(fullscreen);
}

void ClientHandler::NotifyLoadingState(bool isLoading,
                                       bool canGoBack,
                                       bool canGoForward) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&ClientHandler::NotifyLoadingState, this,
                   isLoading, canGoBack, canGoForward));
    return;
  }

  if (delegate_)
    delegate_->OnSetLoadingState(isLoading, canGoBack, canGoForward);
}

void ClientHandler::NotifyDraggableRegions(
    const std::vector<CefDraggableRegion>& regions) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    // Execute this method on the main thread.
    MAIN_POST_CLOSURE(
        base::Bind(&ClientHandler::NotifyDraggableRegions, this, regions));
    return;
  }

  if (delegate_)
    delegate_->OnSetDraggableRegions(regions);
}

void ClientHandler::BuildTestMenu(CefRefPtr<CefMenuModel> model) {
  if (model->GetCount() > 0)
    model->AddSeparator();

  // Build the sub menu.
  CefRefPtr<CefMenuModel> submenu =
      model->AddSubMenu(CLIENT_ID_TESTMENU_SUBMENU, "Context Menu Test");
  submenu->AddCheckItem(CLIENT_ID_TESTMENU_CHECKITEM, "Check Item");
  submenu->AddRadioItem(CLIENT_ID_TESTMENU_RADIOITEM1, "Radio Item 1", 0);
  submenu->AddRadioItem(CLIENT_ID_TESTMENU_RADIOITEM2, "Radio Item 2", 0);
  submenu->AddRadioItem(CLIENT_ID_TESTMENU_RADIOITEM3, "Radio Item 3", 0);

  // Check the check item.
  if (test_menu_state_.check_item)
    submenu->SetChecked(CLIENT_ID_TESTMENU_CHECKITEM, true);

  // Check the selected radio item.
  submenu->SetChecked(
      CLIENT_ID_TESTMENU_RADIOITEM1 + test_menu_state_.radio_item, true);
}

bool ClientHandler::ExecuteTestMenu(int command_id) {
  if (command_id == CLIENT_ID_TESTMENU_CHECKITEM) {
    // Toggle the check item.
    test_menu_state_.check_item ^= 1;
    return true;
  } else if (command_id >= CLIENT_ID_TESTMENU_RADIOITEM1 &&
             command_id <= CLIENT_ID_TESTMENU_RADIOITEM3) {
    // Store the selected radio item.
    test_menu_state_.radio_item = (command_id - CLIENT_ID_TESTMENU_RADIOITEM1);
    return true;
  }

  // Allow default handling to proceed.
  return false;
}

}  // namespace client
