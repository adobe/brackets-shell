// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_CLIENT_APP_H_
#define CEF_TESTS_CEFCLIENT_CLIENT_APP_H_
#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>
#include "include/cef_app.h"

class ClientApp : public CefApp,
                  public CefBrowserProcessHandler,
                  public CefRenderProcessHandler {
 public:
  
  // Interface for renderer delegates. All RenderDelegates must be returned via
  // CreateRenderDelegates. Do not perform work in the RenderDelegate
  // constructor. See CefRenderProcessHandler for documentation.
  class RenderDelegate : public virtual CefBase {
  public:
      virtual void OnRenderThreadCreated(CefRefPtr<ClientApp> app,
                                         CefRefPtr<CefListValue> extra_info) {}
      
      virtual void OnWebKitInitialized(CefRefPtr<ClientApp> app) {}
      
      virtual void OnBrowserCreated(CefRefPtr<ClientApp> app,
                                    CefRefPtr<CefBrowser> browser) {}
      
      virtual void OnBrowserDestroyed(CefRefPtr<ClientApp> app,
                                      CefRefPtr<CefBrowser> browser) {}
      
      virtual bool OnBeforeNavigation(CefRefPtr<ClientApp> app,
                                      CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefRequest> request,
                                      cef_navigation_type_t navigation_type,
                                      bool is_redirect) {
          return false;
      }
      
      virtual void OnContextCreated(CefRefPtr<ClientApp> app,
                                    CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context) {}
      
      virtual void OnContextReleased(CefRefPtr<ClientApp> app,
                                     CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context) {}
      
      virtual void OnUncaughtException(CefRefPtr<ClientApp> app,
                                       CefRefPtr<CefBrowser> browser,
                                       CefRefPtr<CefFrame> frame,
                                       CefRefPtr<CefV8Context> context,
                                       CefRefPtr<CefV8Exception> exception,
                                       CefRefPtr<CefV8StackTrace> stackTrace) {}
      
      
      virtual void OnFocusedNodeChanged(CefRefPtr<ClientApp> app,
                                        CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        CefRefPtr<CefDOMNode> node) {}
      
      // Called when a process message is received. Return true if the message was
      // handled and should not be passed on to other handlers. RenderDelegates
      // should check for unique message names to avoid interfering with each
      // other.
      virtual bool OnProcessMessageReceived(
                                            CefRefPtr<ClientApp> app,
                                            CefRefPtr<CefBrowser> browser,
                                            CefProcessId source_process,
                                            CefRefPtr<CefProcessMessage> message) {
          return false;
      }
  };

  typedef std::set<CefRefPtr<RenderDelegate> > RenderDelegateSet;
  typedef std::map<int32, std::pair< CefRefPtr<CefV8Context>, CefRefPtr<CefV8Value> > > CallbackMap;

  ClientApp();
        
  void AddCallback(int32 id, CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Value> callbackFunction) {
      callback_map_[id] = std::make_pair(context, callbackFunction);
  }

  // Platform-specific methods implemented in client_app_mac/client_app_win
  double GetElapsedMilliseconds();
  CefString GetCurrentLanguage();
  std::string GetExtensionJSSource();
  static CefString AppGetSupportDirectory();
  static CefString AppGetDocumentsDirectory();

private:
  // Creates all of the RenderDelegate objects. Implemented in
  // client_app_delegates.
  static void CreateRenderDelegates(RenderDelegateSet& delegates);

  // Registers custom schemes. Implemented in client_app_delegates.
  static void RegisterCustomSchemes(CefRefPtr<CefSchemeRegistrar> registrar);

  // CefApp methods.
  virtual void OnRegisterCustomSchemes(
      CefRefPtr<CefSchemeRegistrar> registrar) OVERRIDE {
    RegisterCustomSchemes(registrar);
  }
  virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler()
      OVERRIDE { return this; }

  // CefRenderProcessHandler methods.
  virtual void OnWebKitInitialized() OVERRIDE;
  virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefRefPtr<CefV8Context> context) OVERRIDE;
  virtual void OnContextReleased(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefV8Context> context) OVERRIDE;
  
  virtual void OnUncaughtException(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefV8Context> context,
                                   CefRefPtr<CefV8Exception> exception,
                                   CefRefPtr<CefV8StackTrace> stackTrace) OVERRIDE;
  virtual bool OnProcessMessageReceived(
      CefRefPtr<CefBrowser> browser,
      CefProcessId source_process,
      CefRefPtr<CefProcessMessage> message) OVERRIDE;

  // Set of supported RenderDelegates.
  RenderDelegateSet render_delegates_;
                      
  // Set of callbacks
  CallbackMap callback_map_;
					  
  IMPLEMENT_REFCOUNTING(ClientApp);
};

#endif  // CEF_TESTS_CEFCLIENT_CLIENT_APP_H_
