// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "appshell/browser/browser_window.h"

#include "include/base/cef_bind.h"
#include "appshell/browser/main_message_loop.h"

// Brackets specific change
#ifdef OS_LINUX
#include "appshell/command_callbacks.h"
#include "appshell/native_menu_model.h"
#endif

namespace client {

BrowserWindow::BrowserWindow(Delegate* delegate)
    : delegate_(delegate),
      is_closing_(false) {
  DCHECK(delegate_);
}

void BrowserWindow::SetDeviceScaleFactor(float device_scale_factor) {
}

float BrowserWindow::GetDeviceScaleFactor() const {
  return 1.0f;
}

CefRefPtr<CefBrowser> BrowserWindow::GetBrowser() const {
  REQUIRE_MAIN_THREAD();
  return browser_;
}

bool BrowserWindow::IsClosing() const {
  REQUIRE_MAIN_THREAD();
  return is_closing_;
}

void BrowserWindow::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();
  DCHECK(!browser_);
  browser_ = browser;

  delegate_->OnBrowserCreated(browser);
}

void BrowserWindow::OnBrowserClosing(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();
  DCHECK_EQ(browser->GetIdentifier(), browser_->GetIdentifier());
  is_closing_ = true;
}

void BrowserWindow::OnBrowserClosed(CefRefPtr<CefBrowser> browser) {
  REQUIRE_MAIN_THREAD();
  if (browser_.get()) {
    DCHECK_EQ(browser->GetIdentifier(), browser_->GetIdentifier());
    browser_ = NULL;
  }

  client_handler_->DetachDelegate();
  client_handler_ = NULL;

  // |this| may be deleted.
  delegate_->OnBrowserWindowDestroyed();
}

void BrowserWindow::OnSetAddress(const std::string& url) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnSetAddress(url);
}

void BrowserWindow::OnSetTitle(const std::string& title) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnSetTitle(title);
}

void BrowserWindow::OnSetFullscreen(bool fullscreen) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnSetFullscreen(fullscreen);
}

void BrowserWindow::OnSetLoadingState(bool isLoading,
                                      bool canGoBack,
                                      bool canGoForward) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnSetLoadingState(isLoading, canGoBack, canGoForward);
}

void BrowserWindow::OnSetDraggableRegions(
      const std::vector<CefDraggableRegion>& regions) {
  REQUIRE_MAIN_THREAD();
  delegate_->OnSetDraggableRegions(regions);
}

#ifdef OS_LINUX
// Brackets specific change.
// The following is usually called in the multi window workflows.
// Once a window is done processing the "QuitApplication" command, 
// We pass the quit to the next window.
void BrowserWindow::DispatchCloseToBrowser(CefRefPtr<CefBrowser> browser)
{
  if(client_handler_){
    CefRefPtr<CommandCallback> callback = new CloseWindowCommandCallback(browser);
    client_handler_->SendJSCommand(browser, FILE_CLOSE_WINDOW, callback);
  }
}

// The following routine dispatches brackets specific commands to the browser.
void BrowserWindow::DispatchCommandToBrowser(CefRefPtr<CefBrowser> browser, int tag)
{
  if(client_handler_){
    ExtensionString commandId = NativeMenuModel::getInstance(getMenuParent(browser)).getCommandId(tag);
    CefRefPtr<CommandCallback> callback = new EditCommandCallback(browser, commandId);
    client_handler_->SendJSCommand(browser, commandId, callback);
  }
}

#endif

}  // namespace client
