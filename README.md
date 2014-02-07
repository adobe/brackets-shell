## Overview

This is the CEF3-based application shell for [Brackets](https://github.com/adobe/brackets). 

Please read the main [README in the brackets repo](https://github.com/adobe/brackets/blob/master/README.md) 
for general information about Brackets.

If you are interested in contributing to this shell, let us know on the 
[brackets-dev Google Group](http://groups.google.com/group/brackets-dev), 
or on the [#brackets channel on freenode](http://webchat.freenode.net/?channels=brackets).

If you run into any issues with this new shell, please file a bug in the 
[brackets issue tracker](https://github.com/adobe/brackets/issues).

_Note: The brackets-shell is only maintained for use by the Brackets project. Although some people have 
definitely had success using it as an app shell for other projects, we don't provide any official 
support for that and we haven't done a ton of work to make the app shell easily reusable. Many people 
will likely find it easier to use a project like node-webkit, which is more generic by design._

## Running

There are no downloads for the brackets-shell. You either need to 
build from source, or grab the [latest Brackets installer](http://download.brackets.io) 
and run the shell from that.

When the app is launched, it first looks for an index.html file in the following locations:
* Mac - Brackets.app/Contents/dev/src/index.html, Brackets.app/Contents/www/index.html
* Win - dev/src/index.html, www/index.html (these folders must be in the same folder as Brackets.exe)
* Linux - dev/src/index.html, www/index.html (these folders must be in the same folder as the Brackets executable)

If the index.html can't be found, you'll be prompted to find the brackets index.html file. 
Make sure you select the brackets/src/index.html file, and *not* some other file. 

The preferences are stored in `{USER}/Library/Application Support/Brackets/cef_data` on Mac, `{USER}\AppData\Roaming\Brackets\cef_data` on Windows, and `~/.Brackets/cef_data` on Linux.

## Building

Information on building the app shell can be found on the [brackets-shell wiki](https://github.com/adobe/brackets-shell/wiki/Building-brackets-shell).
