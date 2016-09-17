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

#include "appshell/appshell_helpers.h"

#include "include/cef_version.h"
#include "config.h"

namespace appshell {

CefString AppGetProductVersionString() {
    // TODO
    return CefString("");
}

CefString AppGetChromiumVersionString() {
    // TODO
    return CefString("");
}

CefString AppGetSupportDirectory()
{
    gchar *supportDir = g_strdup_printf("%s/%s", g_get_user_config_dir(), APP_NAME);
    CefString result = CefString(supportDir);
    g_free(supportDir);

    return result;
}

CefString AppGetDocumentsDirectory()
{
    const char *dir = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
    if (dir == NULL)  {
        return AppGetSupportDirectory();
    } else {
        return CefString(dir);
    }
}

CefString AppGetCachePath() {
  std::string cachePath = std::string(AppGetSupportDirectory()) + "/cef_data";

  return CefString(cachePath);
}

}  // namespace appshell
