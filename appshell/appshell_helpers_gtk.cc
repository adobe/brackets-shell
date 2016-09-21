
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

#include "appshell_helpers.h"

extern char _binary_appshell_appshell_extensions_js_start;

namespace appshell {

std::string GetExtensionJSSource()
{
    //# We objcopy the appshell/appshell_extensions.js file, and link it directly into the binary.
    //# See http://www.linuxjournal.com/content/embedding-file-executable-aka-hello-world-version-5967
    const char* p = &_binary_appshell_appshell_extensions_js_start;
    std::string content(p);
    // std::string extensionJSPath(AppGetSupportDirectory());
    // extensionJSPath.append("/appshell_extensions.js");
    // FILE* file = fopen(extensionJSPath.c_str(),"r");
    // if(file == NULL)
    // {
    //     return "";
    // }

    // fseek(file, 0, SEEK_END);
    // long int size = ftell(file);
    // rewind(file);

    // char* content = (char*)calloc(size + 1, 1);

    // fread(content,1,size,file);

    return content;
}

void PopulateSettings(CefSettings* settings) {
    settings->no_sandbox = true;
}

}  // namespace appshell
