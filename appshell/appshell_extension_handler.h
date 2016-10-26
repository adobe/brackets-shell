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
 */

#pragma once

#include "include/cef_process_message.h"
#include "include/cef_v8.h"

#include "appshell/appshell_helpers.h"

namespace appshell {

// Forward declarations.
void SetList(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target);
void SetList(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target);

// Transfer a V8 value to a List index.
void SetListValue(CefRefPtr<CefListValue> list, int index,
                  CefRefPtr<CefV8Value> value) {
    if (value->IsArray()) {
        CefRefPtr<CefListValue> new_list = CefListValue::Create();
        SetList(value, new_list);
        list->SetList(index, new_list);
    } else if (value->IsString()) {
        list->SetString(index, value->GetStringValue());
    } else if (value->IsBool()) {
        list->SetBool(index, value->GetBoolValue());
    } else if (value->IsInt()) {
        list->SetInt(index, value->GetIntValue());
    } else if (value->IsDouble()) {
        list->SetDouble(index, value->GetDoubleValue());
    }
}

// Transfer a V8 array to a List.
void SetList(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target) {
    DCHECK(source->IsArray());

    int arg_length = source->GetArrayLength();
    if (arg_length == 0)
        return;

    // Start with null types in all spaces.
    target->SetSize(arg_length);

    for (int i = 0; i < arg_length; ++i)
        SetListValue(target, i, source->GetValue(i));
}

CefRefPtr<CefV8Value> ListValueToV8Value(CefRefPtr<CefListValue> value, int index)
{
    CefRefPtr<CefV8Value> new_value;

    CefValueType type = value->GetType(index);
    switch (type) {
        case VTYPE_LIST: {
            CefRefPtr<CefListValue> list = value->GetList(index);
            new_value = CefV8Value::CreateArray(list->GetSize());
            SetList(list, new_value);
        } break;
        case VTYPE_BOOL:
            new_value = CefV8Value::CreateBool(value->GetBool(index));
            break;
        case VTYPE_DOUBLE:
            new_value = CefV8Value::CreateDouble(value->GetDouble(index));
            break;
        case VTYPE_INT:
            new_value = CefV8Value::CreateInt(value->GetInt(index));
            break;
        case VTYPE_STRING:
            new_value = CefV8Value::CreateString(value->GetString(index));
            break;
        default:
            new_value = CefV8Value::CreateNull();
            break;
    }

    return new_value;
}

// Transfer a List value to a V8 array index.
void SetListValue(CefRefPtr<CefV8Value> list, int index,
                  CefRefPtr<CefListValue> value) {
    CefRefPtr<CefV8Value> new_value;

    CefValueType type = value->GetType(index);
    switch (type) {
        case VTYPE_LIST: {
            CefRefPtr<CefListValue> listValue = value->GetList(index);
            new_value = CefV8Value::CreateArray(listValue->GetSize());
            SetList(listValue, new_value);
        } break;
        case VTYPE_BOOL:
            new_value = CefV8Value::CreateBool(value->GetBool(index));
            break;
        case VTYPE_DOUBLE:
            new_value = CefV8Value::CreateDouble(value->GetDouble(index));
            break;
        case VTYPE_INT:
            new_value = CefV8Value::CreateInt(value->GetInt(index));
            break;
        case VTYPE_STRING:
            new_value = CefV8Value::CreateString(value->GetString(index));
            break;
        default:
            break;
    }

    if (new_value.get()) {
        list->SetValue(index, new_value);
    } else {
        list->SetValue(index, CefV8Value::CreateNull());
    }
}

// Transfer a List to a V8 array.
void SetList(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target) {
    DCHECK(target->IsArray());

    int arg_length = source->GetSize();
    if (arg_length == 0)
        return;

    for (int i = 0; i < arg_length; ++i)
        SetListValue(target, i, source);
}


// Handles the native implementation for the appshell extension.
class AppShellExtensionHandler : public CefV8Handler {
  public:
    explicit AppShellExtensionHandler(CefRefPtr<ClientApp> client_app)
      : client_app_(client_app)
      , messageId(0) {
    }

    virtual bool Execute(const CefString& name,
                         CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval,
                         CefString& exception) {

        // The only messages that are handled here is getElapsedMilliseconds(),
        // GetCurrentLanguage(), GetApplicationSupportDirectory(), and GetRemoteDebuggingPort().
        // All other messages are passed to the browser process.
        if (name == "GetElapsedMilliseconds") {
            retval = CefV8Value::CreateDouble(GetElapsedMilliseconds());
        } else if (name == "GetCurrentLanguage") {
            retval = CefV8Value::CreateString(GetCurrentLanguage());
        } else if (name == "GetApplicationSupportDirectory") {
            retval = CefV8Value::CreateString(AppGetSupportDirectory());
        } else if (name == "GetUserDocumentsDirectory") {
            retval = CefV8Value::CreateString(AppGetDocumentsDirectory());
        } else if (name == "GetRemoteDebuggingPort") {
            retval = CefV8Value::CreateInt(REMOTE_DEBUGGING_PORT);
        } else {
            // Pass all messages to the browser process. Look in appshell_extensions.cpp for implementation.
            CefRefPtr<CefBrowser> browser = CefV8Context::GetCurrentContext()->GetBrowser();
            if (!browser.get()) {
                // If we don't have a browser, we can't handle the command.
                return false;
            }
            CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(name);
            CefRefPtr<CefListValue> messageArgs = message->GetArgumentList();

            // The first argument must be a callback function
            if (arguments.size() > 0 && !arguments[0]->IsFunction()) {
                std::string functionName = name;
                fprintf(stderr, "Function called without callback param: %s\n", functionName.c_str());
                return false;
            }

            if (arguments.size() > 0) {
                // The first argument is the message id
                client_app_->AddCallback(messageId, CefV8Context::GetCurrentContext(), arguments[0]);
                SetListValue(messageArgs, 0, CefV8Value::CreateInt(messageId));
            }

            // Pass the rest of the arguments
            for (unsigned int i = 1; i < arguments.size(); i++)
                SetListValue(messageArgs, i, arguments[i]);
            browser->SendProcessMessage(PID_BROWSER, message);

            messageId++;
        }

        return true;
    }

  private:
    CefRefPtr<ClientApp> client_app_;
    int32 messageId;

    IMPLEMENT_REFCOUNTING(AppShellExtensionHandler);
};

// Simple stack class to ensure calls to Enter and Exit are balanced.
class StContextScope {
  public:
    StContextScope(const CefRefPtr<CefV8Context>& ctx)
      : m_ctx(NULL) {
        if (ctx && ctx->Enter()) {
            m_ctx = ctx;
        }
    }

    ~StContextScope() {
        if (m_ctx) {
            m_ctx->Exit();
        }
    }

    const CefRefPtr<CefV8Context>& GetContext() const {
        return m_ctx;
    }

  private:
    CefRefPtr<CefV8Context> m_ctx;
};

}  // namespace
