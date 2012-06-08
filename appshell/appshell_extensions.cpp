/*
 * Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
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

#include "appshell/appshell_extensions.h"

namespace appshell_extensions {

namespace {
class ProcessMessageDelegate : public ClientHandler::ProcessMessageDelegate {
public:
    ProcessMessageDelegate()
    {
    }

    // From ClientHandler::ProcessMessageDelegate.
    virtual bool OnProcessMessageRecieved(
                                          CefRefPtr<ClientHandler> handler,
                                          CefRefPtr<CefBrowser> browser,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message) OVERRIDE {
        std::string message_name = message->GetName();
        
        return false;
    }
    
    IMPLEMENT_REFCOUNTING(ProcessMessageDelegate);
};
    
} // namespace
    
void CreateProcessMessageDelegates(
                                   ClientHandler::ProcessMessageDelegateSet& delegates) {
    delegates.insert(new ProcessMessageDelegate);
}

void CreateRequestDelegates(ClientHandler::RequestDelegateSet& delegates) {
//	delegates.insert(new RequestDelegate);
}
    
} // namespace appshell_extensions
