// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

// This file is shared by cefclient and cef_unittests so don't include using
// a qualified path.
#include "client_app.h"  // NOLINT(build/include)

#include <string>

#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"
#include "include/base/cef_logging.h"
#include "config.h"
#include "appshell/appshell_extension_handler.h"
#include "appshell/appshell_helpers.h"

#ifdef OS_WIN
extern bool g_force_enable_acc;
#endif

ClientApp::ClientApp() {
  CreateRenderDelegates(render_delegates_);
}

void ClientApp::OnWebKitInitialized() {
  // Register the appshell extension.
  std::string extension_code = appshell::GetExtensionJSSource();

  CefRegisterExtension("appshell", extension_code,
      new appshell::AppShellExtensionHandler(this));

  // Execute delegate callbacks.
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnWebKitInitialized(this);
}

void ClientApp::OnContextCreated(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               CefRefPtr<CefV8Context> context) {
  // Execute delegate callbacks.
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnContextCreated(this, browser, frame, context);
}

void ClientApp::OnBeforeCommandLineProcessing(
	const CefString& process_type,
	CefRefPtr<CefCommandLine> command_line)
{
 #ifdef OS_WIN
  // Check if the user wants to enable renderer accessibility
  // and if not, then disable renderer accessibility.
  if (!g_force_enable_acc)
    command_line->AppendSwitch("disable-renderer-accessibility");
 #endif
}

void ClientApp::OnBeforeChildProcessLaunch(
	CefRefPtr<CefCommandLine> command_line)
{
#ifdef OS_WIN
  if (!g_force_enable_acc)
    command_line->AppendSwitch("disable-renderer-accessibility");
#endif
}

void ClientApp::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefRefPtr<CefV8Context> context) {
  // Execute delegate callbacks.
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnContextReleased(this, browser, frame, context);
 
  // This is to fix the crash on quit(https://github.com/adobe/brackets/issues/7683) 
  // after integrating CEF 2171.

  // On Destruction, callback_map_ was getting destroyed
  // in the ClientApp::~ClientApp(). However while removing
  // all the elements, it was trying to destroy some stale
  // objects which were already deleted. So to fix this, we
  // are now explicitly clearing the map here.

  CallbackMap::iterator iCallBack = callback_map_.begin();
  for (; iCallBack != callback_map_.end();) {
  if (iCallBack->second.first->IsSame(context))
    callback_map_.erase(iCallBack++);
  else
    ++iCallBack;
  }
    
}

void ClientApp::OnUncaughtException(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Exception> exception,
                                    CefRefPtr<CefV8StackTrace> stackTrace) {
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end(); ++it) {
        (*it)->OnUncaughtException(this, browser, frame, context, exception,
                                   stackTrace);
    }
}

bool ClientApp::OnProcessMessageReceived(
        CefRefPtr<CefBrowser> browser,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) {
    DCHECK(source_process == PID_BROWSER);

    bool handled = false;

    // Execute delegate callbacks.
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end() && !handled; ++it) {
        handled = (*it)->OnProcessMessageReceived(this, browser, source_process, message);
    }

    if (!handled) {
        if (message->GetName() == "invokeCallback") {
            // This is called by the appshell extension handler to invoke the asynchronous 
            // callback function
            
            CefRefPtr<CefListValue> messageArgs = message->GetArgumentList();
            int32 callbackId = messageArgs->GetInt(0);
                    
            CefRefPtr<CefV8Context> context = callback_map_[callbackId].first;
            CefRefPtr<CefV8Value> callbackFunction = callback_map_[callbackId].second;
            CefV8ValueList arguments;
            context->Enter();
            
            // Sanity check to make sure the context is still attched to a browser.
            // Async callbacks could be initiated after a browser instance has been deleted,
            // which can lead to bad things. If the browser instance has been deleted, don't
            // invoke this callback. 
            if (context->GetBrowser()) {
                for (size_t i = 1; i < messageArgs->GetSize(); i++) {
                    arguments.push_back(appshell::ListValueToV8Value(messageArgs, i));
                }
                
                callbackFunction->ExecuteFunction(NULL, arguments);
            }
            
            context->Exit();
            
            callback_map_.erase(callbackId);
        } else if (message->GetName() == "executeCommand") {
            // This is called by the browser process to execute a command via JavaScript
            // 
            // The first argument is the command name. This is required.
            // The second argument is a message id. This is optional. If set, a response
            // message will be sent back to the browser process.
            
            CefRefPtr<CefListValue> messageArgs = message->GetArgumentList();
            CefString commandName = messageArgs->GetString(0);
            int messageId = messageArgs->GetSize() > 1 ? messageArgs->GetInt(1) : -1;
            handled = false;
            
            appshell::StContextScope ctx(browser->GetMainFrame()->GetV8Context());
            
            CefRefPtr<CefV8Value> global = ctx.GetContext()->GetGlobal();
            
            if (global->HasValue("brackets")) { 
                
                CefRefPtr<CefV8Value> brackets = global->GetValue("brackets");
                
                if (brackets->HasValue("shellAPI")) {
                    
                    CefRefPtr<CefV8Value> shellAPI = brackets->GetValue("shellAPI");
                    
                    if (shellAPI->HasValue("executeCommand")) {
                        
                        CefRefPtr<CefV8Value> executeCommand = shellAPI->GetValue("executeCommand");
                        
                        if (executeCommand->IsFunction()) {
                            
                            CefRefPtr<CefV8Value> retval;
                            CefV8ValueList args;
                            args.push_back(CefV8Value::CreateString(commandName));
                            
                            retval = executeCommand->ExecuteFunction(global, args);
                            
                            if (retval) {
                                handled = retval->GetBoolValue();
                            }
                        }
                    }
                }
            }
            
            // Return a message saying whether or not the command was handled
            if (messageId != -1) {
                CefRefPtr<CefProcessMessage> result = CefProcessMessage::Create("executeCommandCallback");
                result->GetArgumentList()->SetInt(0, messageId);
                result->GetArgumentList()->SetBool(1, handled);
                
                browser->SendProcessMessage(PID_BROWSER, result);
            }
        }
    }
    
    return handled;
}
