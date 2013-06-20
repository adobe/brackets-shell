/*
 * Copyright (c) 2013 Adobe Systems Incorporated. All rights reserved.
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

// IDs of static menuitems
#define BRACKETS_MENUITEMTAG      0
#define WINDOW_MENUITEMTAG     4999


class NativeMenuItemModel
{
    public:
    NativeMenuItemModel() :
        checked(false),
        enabled(false),
        osItem(NULL)
    {
    }
    NativeMenuItemModel(const ExtensionString& commandId, const ExtensionString& parentId, bool enabled, bool checked) :
        checked(checked),
        enabled(enabled),
        osItem(NULL),
        commandId(commandId),
        parentId(parentId)
    {
    }
    bool checked;
    bool enabled;
    void *osItem;
    ExtensionString commandId;
    ExtensionString parentId;
};

//command name -> menutag
typedef std::map<ExtensionString, int> menuTag;

//menu tag to item model
typedef std::map<int, NativeMenuItemModel> menu;

//tag id for the first native menu item. This is incremented for each new item.
const int kInitialTagCount = 5001;

//value for tags that cannot be found
const int kTagNotFound = -1;

class NativeMenuModel {
private:
    menu menuItems;
    menuTag commandMap;
    int tagCount;
    NativeMenuModel(menu items) :
        menuItems(items),
        tagCount(kInitialTagCount)
    {
    }
public:
    //not at all threadsafe
    static void resetMenus(void* menuParent) { NativeMenuModel::getInstance(menuParent, true); };
    static NativeMenuModel& getInstance(void* menuParent, bool reset = false);
    
    bool isMenuItemEnabled(int tag);
    bool isMenuItemChecked(int tag);
    int setMenuItemState(ExtensionString command, bool enabled, bool checked);
    int getOrCreateTag(ExtensionString command, ExtensionString parent);
    int getTag(ExtensionString command);
    int setTag(ExtensionString command, ExtensionString parent, int tag);
    ExtensionString getCommandId(int tag);
    ExtensionString getParentId(int tag);
    void setOsItem (int tag, void* theItem);
    void* getOsItem (int tag);
    
    int removeMenuItem(const ExtensionString& command);
};





