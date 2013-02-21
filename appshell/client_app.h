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
                  public CefRenderProcessHandler {
 public:
  // Interface for renderer delegates. All RenderDelegates must be returned via
  // CreateRenderDelegates. Do not perform work in the RenderDelegate
  // constructor. See CefRenderProcessHandler for documentation.
  class RenderDelegate : public virtual CefBase {

   public:
    // Called when WebKit is initialized. Used to register V8 extensions.
    virtual void OnWebKitInitialized(CefRefPtr<ClientApp> app) {
    };

    // Called when a V8 context is created. Used to create V8 window bindings
    // and set message callbacks. RenderDelegates should check for unique URLs
    // to avoid interfering with each other.
    virtual void OnContextCreated(CefRefPtr<ClientApp> app,
                                  CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context) {
    };

    // Called when a V8 context is released. Used to clean up V8 window
    // bindings. RenderDelegates should check for unique URLs to avoid
    // interfering with each other.
    virtual void OnContextReleased(CefRefPtr<ClientApp> app,
                                   CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefV8Context> context) {
    };

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
        
  // Set the proxy configuration. Should only be called during initialization.
  void SetProxyConfig(cef_proxy_type_t proxy_type,
                      const CefString& proxy_config) {
    proxy_type_ = proxy_type;
    proxy_config_ = proxy_config;
  }
  
  void AddCallback(int32 id, CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Value> callbackFunction) {
      callback_map_[id] = std::make_pair(context, callbackFunction);
  }

  // Platform-specific methods implemented in client_app_mac/client_app_win
  double GetElapsedMilliseconds();
  CefString GetCurrentLanguage();
  std::string GetExtensionJSSource();
  static CefString AppGetSupportDirectory();

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
  
  virtual bool OnProcessMessageReceived(
      CefRefPtr<CefBrowser> browser,
      CefProcessId source_process,
      CefRefPtr<CefProcessMessage> message) OVERRIDE;

  // Proxy configuration.
  cef_proxy_type_t proxy_type_;
  CefString proxy_config_;
  
  // Set of supported RenderDelegates.
  RenderDelegateSet render_delegates_;
                      
  // Set of callbacks
  CallbackMap callback_map_;
					  
  IMPLEMENT_REFCOUNTING(ClientApp);
};

#endif  // CEF_TESTS_CEFCLIENT_CLIENT_APP_H_
