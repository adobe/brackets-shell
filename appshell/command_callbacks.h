#pragma once

#include "include/cef_browser.h"
#include "appshell_extensions_platform.h"

// Command names. These MUST be in sync with the command names in
// brackets/src/command/Commands.js
const std::string FILE_QUIT = "file.quit";
const std::string FILE_CLOSE_WINDOW = "file.close_window";
const std::string HELP_ABOUT = "help.about";

// Base CommandCallback class
class CommandCallback : public CefBase {
  
public:
  // Called when the command is complete. When handled=true, the command
  // was handled and no further processing should occur.
  virtual void CommandComplete(bool handled) = 0;
  
private:
  IMPLEMENT_REFCOUNTING(CommandCallback);
};

// CommandCallback subclasses

// CloseWindowCommandCallback
class CloseWindowCommandCallback : public CommandCallback {
public:
  CloseWindowCommandCallback(CefRefPtr<CefBrowser> browser)
  : browser_(browser) {
    BringBrowserWindowToFront(browser_);
  }
  
  virtual void CommandComplete(bool handled) {
    if (!handled) {
      // If the command returned false, close the window
      CloseWindow(browser_);
    }
  }
private:
  CefRefPtr<CefBrowser> browser_;
};
