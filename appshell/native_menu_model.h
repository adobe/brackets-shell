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

#pragma once

#include <map>
#include <string>
#include "appshell_extensions_platform.h"

class NativeMenuItemModel
{
    public:
    NativeMenuItemModel() {
        this->enabled = false;
        this->checked = false;
        this->osItem = NULL;
    }
    NativeMenuItemModel(const ExtensionString& commandId, bool enabled, bool checked) {
        this->commandId = commandId;
        this->enabled = enabled;
        this->checked = checked;
        this->osItem = NULL;
    }
    bool checked;
    bool enabled;
    void *osItem;
    ExtensionString commandId;
};

//command name -> menutag
typedef std::map<ExtensionString, int> menuTag;

//menu tag to item model
typedef std::map<int, NativeMenuItemModel> menu;


class NativeMenuModel {
private:
    menu menuItems;
    menuTag commandMap;
    int tagCount;
    NativeMenuModel(menu items) {
        tagCount = 5001;
        menuItems = items;
    }
public:
    //not at all threadsafe
    static void resetMenus(void* menuParent) { NativeMenuModel::getInstance(menuParent, true); };
    static NativeMenuModel& getInstance(void* menuParent, bool reset = false);
    
    bool isMenuItemEnabled(int tag);
    bool isMenuItemChecked(int tag);
    int setMenuItemState(ExtensionString command, bool enabled, bool checked);
    int getOrCreateTag(ExtensionString command);
    int getTag(ExtensionString command);
    int setTag(ExtensionString command, int tag);
    ExtensionString getCommandId(int tag);
	void setOsItem (int tag, void* theItem);
	void* getOsItem (int tag);
    
    int removeMenuItem(const ExtensionString& command);
};





