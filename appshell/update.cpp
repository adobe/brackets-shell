
/*
 * Copyright (c) 2018 - present Adobe Systems Incorporated. All rights reserved.
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

// This file contains the implementation for functions common to Win and Mac Auto Update.

#include "update.h"


#if defined(OS_LINUX)

//In Linux, all the shell side functions will be no-op for now

int32 ParseCommandLineParamsJSON(CefString &updateArgs, CefRefPtr<CefDictionaryValue> &argsDict) { return NO_ERROR; }

int32 SetInstallerCommandLineArgs(CefString &updateArgs) { return NO_ERROR; }

int32 RunAppUpdate() { return NO_ERROR;  }

#else

// Parses the update parameters json from a cef string to a cef dictionary
int32 ParseCommandLineParamsJSON(CefString &updateArgs, CefRefPtr<CefDictionaryValue> &argsDict)
{
	if (updateArgs.length()) {
		CefRefPtr<CefValue> argsJSON = CefParseJSON(updateArgs, JSON_PARSER_RFC);
		if (argsJSON->IsValid())
		{
			CefValueType argType = argsJSON->GetType();
			if (argType == VTYPE_DICTIONARY)
			{
				CefRefPtr<CefDictionaryValue> argsDictionary = argsJSON->GetDictionary();
				if (argsDictionary)
				{
					argsDict = argsDictionary;
					return NO_ERROR;
				}
			}
		}

	}
	return ERR_UPDATE_ARGS_INIT_FAILED;
}

#endif //OS_LINUX