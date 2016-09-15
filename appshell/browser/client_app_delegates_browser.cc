// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "appshell/browser/client_app_browser.h"

#if defined(OS_LINUX)
#include "appshell/browser/print_handler_gtk.h"
#endif

namespace client {

// static
void ClientAppBrowser::CreateDelegates(DelegateSet& delegates) {
}

// static
CefRefPtr<CefPrintHandler> ClientAppBrowser::CreatePrintHandler() {
#if defined(OS_LINUX)
  return new ClientPrintHandlerGtk();
#else
  return NULL;
#endif
}

}  // namespace client
