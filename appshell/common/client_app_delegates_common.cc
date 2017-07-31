// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "appshell/common/client_app.h"
//#include "appshell/common/scheme_test_common.h"

namespace client {

// static
void ClientApp::RegisterCustomSchemes(
    CefRefPtr<CefSchemeRegistrar> registrar,
    std::vector<CefString>& cookiable_schemes) {
  // Brackets specific change.
  //scheme_test::RegisterCustomSchemes(registrar, cookiable_schemes);
}

}  // namespace client
