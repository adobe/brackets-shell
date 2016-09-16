/*
 * Copyright (c) 2016 - present Adobe Systems Incorporated. All rights reserved.
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

#pragma once

#include "include/cef_browser.h"
#include "appshell_extensions_platform.h"
#include "config.h"

// Command names. These MUST be in sync with the command names in
// brackets/src/command/Commands.js
const std::string APP_ABORT_QUIT        = "app.abort_quit";
const std::string APP_BEFORE_MENUPOPUP  = "app.before_menupopup";
const std::string FILE_QUIT             = "file.quit";
const std::string FILE_CLOSE_WINDOW     = "file.close_window";
const std::string HELP_ABOUT            = "help.about";
const std::string OPEN_PREFERENCES_FILE = "debug.openPrefsInSplitView";

#if defined(OS_WIN)
const ExtensionString EDIT_UNDO         = L"edit.undo";
const ExtensionString EDIT_REDO         = L"edit.redo";
const ExtensionString EDIT_CUT          = L"edit.cut";
const ExtensionString EDIT_COPY         = L"edit.copy";
const ExtensionString EDIT_PASTE        = L"edit.paste";
const ExtensionString EDIT_SELECT_ALL   = L"edit.selectAll";
#else
const ExtensionString EDIT_UNDO         = "edit.undo";
const ExtensionString EDIT_REDO         = "edit.redo";
const ExtensionString EDIT_CUT          = "edit.cut";
const ExtensionString EDIT_COPY         = "edit.copy";
const ExtensionString EDIT_PASTE        = "edit.paste";
const ExtensionString EDIT_SELECT_ALL   = "edit.selectAll";
#endif

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

class EditCommandCallback : public CommandCallback {
public:
    EditCommandCallback(CefRefPtr<CefBrowser> browser, ExtensionString commandId)
    : browser_(browser)
    , commandId_(commandId) {
    }
    
    virtual void CommandComplete(bool handled) {
        if (!handled) {
            CefRefPtr<CefFrame> frame = browser_->GetFocusedFrame();
            if (!frame)
                return;
            
            if (commandId_ == EDIT_UNDO) {
                frame->Undo();
            } else if (commandId_ == EDIT_REDO) {
                frame->Redo();
            } else if (commandId_ == EDIT_CUT) {
                frame->Cut();
            } else if (commandId_ == EDIT_COPY) {
                frame->Copy();
            } else if (commandId_ == EDIT_PASTE) {
                frame->Paste();
            } else if (commandId_ == EDIT_SELECT_ALL) {
                frame->SelectAll();
            }
        }
    }
private:
    CefRefPtr<CefBrowser> browser_;
    ExtensionString commandId_;
};
