// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "appshell/renderer/client_app_renderer.h"

#include "include/base/cef_logging.h"
#include "appshell/appshell_extension_handler.h"

namespace client {

ClientAppRenderer::ClientAppRenderer() {
  CreateDelegates(delegates_);
}

void ClientAppRenderer::OnRenderThreadCreated(
    CefRefPtr<CefListValue> extra_info) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnRenderThreadCreated(this, extra_info);
}

void ClientAppRenderer::OnWebKitInitialized() {
  // Register the appshell extension.
  std::string extension_code = GetExtensionJSSource();

  CefRegisterExtension("appshell", extension_code,
      new appshell::AppShellExtensionHandler(this));

  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnWebKitInitialized(this);
}

void ClientAppRenderer::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnBrowserCreated(this, browser);
}

void ClientAppRenderer::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnBrowserDestroyed(this, browser);
}

CefRefPtr<CefLoadHandler> ClientAppRenderer::GetLoadHandler() {
  CefRefPtr<CefLoadHandler> load_handler;
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end() && !load_handler.get(); ++it)
    load_handler = (*it)->GetLoadHandler(this);

  return load_handler;
}

bool ClientAppRenderer::OnBeforeNavigation(CefRefPtr<CefBrowser> browser,
                                           CefRefPtr<CefFrame> frame,
                                           CefRefPtr<CefRequest> request,
                                           NavigationType navigation_type,
                                           bool is_redirect) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it) {
    if ((*it)->OnBeforeNavigation(this, browser, frame, request,
                                  navigation_type, is_redirect)) {
      return true;
    }
  }

  return false;
}

void ClientAppRenderer::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         CefRefPtr<CefV8Context> context) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnContextCreated(this, browser, frame, context);
}

void ClientAppRenderer::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefRefPtr<CefV8Context> context) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
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

void ClientAppRenderer::OnUncaughtException(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context,
    CefRefPtr<CefV8Exception> exception,
    CefRefPtr<CefV8StackTrace> stackTrace) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it) {
    (*it)->OnUncaughtException(this, browser, frame, context, exception,
                               stackTrace);
  }
}

void ClientAppRenderer::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                             CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefDOMNode> node) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnFocusedNodeChanged(this, browser, frame, node);
}

bool ClientAppRenderer::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  DCHECK_EQ(source_process, PID_BROWSER);

  bool handled = false;

  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end() && !handled; ++it) {
    handled = (*it)->OnProcessMessageReceived(this, browser, source_process,
                                              message);
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

}  // namespace client
