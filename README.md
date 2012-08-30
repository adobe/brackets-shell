## Overview

This is the new CEF3-based application shell for [Brackets](https://github.com/adobe/brackets). This replaces the old [brackets-app](https://github.com/adobe/brackets-app) shell.

Please read the main [README in the brackets repo](https://github.com/adobe/brackets/blob/master/README.md) for general information about Brackets.

If you are interested in contributing to this shell, let us know on the [brackets-dev Google Group](http://groups.google.com/group/brackets-dev), or on the [#brackets channel on freenode](http://webchat.freenode.net/?channels=brackets).

If you run into any issues with this new shell, please file a bug in the [brackets issue tracker](https://github.com/adobe/brackets/issues).

## Running

[Download](https://github.com/adobe/brackets-shell/downloads) the .zip file for your platform (there are separate downloads for Mac and Win). 

NOTE: The downloads do **not** contain the html/css/javascript files used for Brackets. You will need to get those separately by cloning (or downloading) the [brackets repo](https://github.com/adobe/brackets).

When the app is launched, it first looks for an index.html file in the following location:
* Mac - Brackets.app/Contents/www/index.html
* Win - www/index.html (the www folder must be in the same folder as Brackets.exe)

If the index.html can't be found, you'll be prompted to find the brackets index.html file. Make sure you select the brackets/src/index.html file, and *not* some other file. This location is remembered for subsequent launches. If you want to point to a *different* index.html file, hold down the shift key while launching and you will get the prompt again. If you want to clear the saved file location, hold down the Option (mac) or Control (win) key while launching. 

NOTE: You need to hold down these modifier keys **before** launching Brackets. If you get a security dialog, make sure you continue to hold the modifier key after dismissing the dialog.

The preferences are stored in `{USER}/Library/Application Support/Brackets/cef_data` on the mac and `{USER}\AppData\Roaming\Brackets\cef_data` on Windows.

## Building

Information on building the app shell can be found on the [brackets-shell wiki](https://github.com/adobe/brackets-shell/wiki/Building-brackets-shell).
