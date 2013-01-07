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
 */
#include "native_menu_model.h"
#include "command_callbacks.h"

//id of the existing window menu item
#define WINDOW_MENUITEMTAG 4999
#if defined(OS_WIN)
const ExtensionString WINDOW_COMMAND    = L"window";
#else
const ExtensionString WINDOW_COMMAND    = "window";
#endif

NativeMenuModel& NativeMenuModel::getInstance(bool reset)
{
    static menu m;
    static NativeMenuModel instance(m);
    if (reset) {
        instance.commandMap.clear();
        instance.menuItems.clear();
    }
    instance.setTag(WINDOW_COMMAND, WINDOW_MENUITEMTAG);
    return instance;
}


bool NativeMenuModel::isMenuItemEnabled(int tag) {
    menu::iterator foundItem = menuItems.find(tag);
    if(foundItem == menuItems.end()) {
        //return enabled
        return true;
    }
    return foundItem->second.enabled;
}

bool NativeMenuModel::isMenuItemChecked(int tag) {
    menu::iterator foundItem = menuItems.find(tag);
    if(foundItem == menuItems.end()) {
        return false;
    }
    return foundItem->second.checked;
}

int NativeMenuModel::setMenuItemState (ExtensionString command, bool enabled, bool checked) {
    //woo turning O(log n) into O(n) however, the important look up is the tag -> menu so making sure that is faster as it will impact user performance.
    menu::iterator it;
    for ( it=menuItems.begin() ; it != menuItems.end(); it++ ) {
        if(it->second.commandId.compare(command) == 0) {
            it->second.enabled = enabled;
            it->second.checked = checked;
            return NO_ERROR;
        }
    }
    return ERR_NOT_FOUND;
}

ExtensionString NativeMenuModel::getCommandId(int tag) {
    return menuItems[tag].commandId;
}

int NativeMenuModel::getOrCreateTag(ExtensionString command)
{
    menuTag::iterator foundItem = commandMap.find(command);
    if(foundItem == commandMap.end()) {
        commandMap[command] = ++tagCount;
        menuItems[tagCount] = NativeMenuItemModel(command, true, false);
        return tagCount;
    }
    return foundItem->second;
}

int NativeMenuModel::setTag(ExtensionString command, int tag)
{
    menuTag::iterator foundItem = commandMap.find(command);
    if(foundItem == commandMap.end()) {
        commandMap[command] = tag;
        menuItems[tag] = NativeMenuItemModel(command, true, false);
        return tagCount;
    }
    return foundItem->second;
}

int NativeMenuModel::getTag(ExtensionString command)
{
    menuTag::iterator foundItem = commandMap.find(command);
    if(foundItem == commandMap.end()) {
        return -1;
    }
    return foundItem->second;
}

void NativeMenuModel::setOsItem (int tag, void* theItem) {
    menuItems[tag].osItem = theItem;
}

void* NativeMenuModel::getOsItem (int tag) {
	 return menuItems[tag].osItem;
}

int NativeMenuModel::removeMenuItem(const ExtensionString& command)
{
    menuTag::iterator foundItem = commandMap.find(command);
    if(foundItem == commandMap.end()) {
        return ERR_NOT_FOUND;
    }
    menuItems.erase(foundItem->second);
    commandMap.erase(foundItem);
    return NO_ERROR;
}
