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
 */
#include "native_menu_model.h"
#include "command_callbacks.h"

#if defined(OS_WIN)
const ExtensionString WINDOW_COMMAND    = L"window";
#else
const ExtensionString WINDOW_COMMAND    = "window";
#endif

// map of menuParent --> NativeMenuModel instance
typedef std::map<void*, NativeMenuModel*> menuModelMap;
menuModelMap instanceMap;

NativeMenuModel& NativeMenuModel::getInstance(void* menuParent, bool reset)
{
    menuModelMap::iterator foundItem = instanceMap.find(menuParent);

    if (foundItem != instanceMap.end()) {
        return *(foundItem->second);
    }

    menu m;
    NativeMenuModel* instance = new NativeMenuModel(m);
    instance->setTag(WINDOW_COMMAND, ExtensionString(), WINDOW_MENUITEMTAG);
    instanceMap[menuParent] = instance;
    return *(instance);
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
    menu::iterator foundItem = menuItems.find(tag);
    if(foundItem == menuItems.end()) {
        return ExtensionString();
    }
    return menuItems[tag].commandId;
}

ExtensionString NativeMenuModel::getParentId(int tag) {
    menu::iterator foundItem = menuItems.find(tag);
    if(foundItem == menuItems.end()) {
        return ExtensionString();
    }
    return menuItems[tag].parentId;
}

int NativeMenuModel::getOrCreateTag(ExtensionString command, ExtensionString parent)
{
    menuTag::iterator foundItem = commandMap.find(command);
    if(foundItem == commandMap.end()) {
        commandMap[command] = ++tagCount;
        menuItems[tagCount] = NativeMenuItemModel(command, parent, true, false);
        return tagCount;
    }
    return foundItem->second;
}

int NativeMenuModel::setTag(ExtensionString command, ExtensionString parent, int tag)
{
    menuTag::iterator foundItem = commandMap.find(command);
    if(foundItem == commandMap.end()) {
        commandMap[command] = tag;
        menuItems[tag] = NativeMenuItemModel(command, parent, true, false);
        return tagCount;
    }
    return foundItem->second;
}

int NativeMenuModel::getTag(ExtensionString command)
{
    menuTag::iterator foundItem = commandMap.find(command);
    if(foundItem == commandMap.end()) {
        return kTagNotFound;
    }
    return foundItem->second;
}

void NativeMenuModel::setOsItem (int tag, void* theItem) {
    menu::iterator foundItem = menuItems.find(tag);
    if(foundItem == menuItems.end()) {
        return;
    }
    menuItems[tag].osItem = theItem;
}

void* NativeMenuModel::getOsItem (int tag) {
    menu::iterator foundItem = menuItems.find(tag);
    if(foundItem == menuItems.end()) {
        return NULL;
    }
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
