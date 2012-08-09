## Overview

This is the beginnings of a new CEF3-based application shell for Brackets. It is **not** ready for use yet, so please continue to use the existing [brackets-app](https://github.com/adobe/brackets-app) shell until further notice.

If you are interested in contributing to this shell, let us know on the [brackets-dev Google Group](http://groups.google.com/group/brackets-dev), or on the [#brackets channel on freenode](http://webchat.freenode.net/?channels=brackets).

## Running

[Download](https://github.com/adobe/brackets-shell/downloads) the .zip file for your platform (there are separate downloads for Mac and Win). 

The first time the app is run it will ask you to find the brackets index.html file. This location is remembered for subsequent launches. If you want to point to a *different* index.html file, hold down the shift key while launching and you will get the prompt again.

The preferences are stored in `{USER}/Library/Application Support/Brackets/cef_data` on the mac and `{USER}\AppData\Roaming\Brackets\cef_data` on Windows.

## Building

This project requires a CEF3 binary distribution in order to build.

### Mac
####Prerequisites

* XCode 3.2.6 - 4.4 required to build the project
* CEF3 binary distribution version 3.1180.719 or newer
* To modify the project files, you will also need:
  * python
  * chromium source code (at least the src/build and src/tools directories). Hopefully this is a short-term requirement.

####Setup and Building
Create a folder named `deps` inside the `brackets-shell` folder.
Create a folder named `cef` inside the `deps` folder.
Copy all of the contents of the CEF3 binary distribution into the `deps/cef` directory. 

Your directory structure should look like this:
```
brackets-shell
   deps
      cef
         // CEF3 binary content in this folder
   appshell
      // appshell source
   README.md
   ...
```

Open a terminal window on this directory and run `./make_symlinks.sh`. This will create symbolic links to several folders in the `deps/cef` directory.
Open appshell.pbxproj in XCode. NOTE: If you are using XCode 4.4, you will get a couple warnings. These are harmless, and will be fixed soon.

####Generating Projects
This is only required if you are changing the project files. **NOTE:** Don't change the xcode project files directly. Any changes should be done to the .gyp files, and new xcode projects should be generated.

* Add a <code>CHROMIUM\_SRC\_PATH</code> environment variable that points to your chromium 'src' folder (without a final '/').
* Open a terminal window on this directory and run <code>./make\_appshell\_project.sh</code> (Note: while not required, it is a good idea to delete the old appshell.xcodeproj before generating a new one.)

### Windows

####Prerequisites

* Visual Studio 2010 or later required to build the project. The free Visual Studio Express works fine.
* CEF3 binary distribution version 3.1180.719 or newer
* To modify the project files, you will also need:
  * python
  * chromium source code (at least the src/build and src/tools directories). Hopefully this is a short-term requirement.

####Setup and Building
Create a folder named `deps` inside the `brackets-shell` folder.
Create a folder named `cef` inside the `deps` folder.
Copy all of the contents of the CEF3 binary distribution into the `deps/cef` directory. 

Your directory structure should look like this:
```
brackets-shell
   deps
      cef
         // CEF3 binary content in this folder
   appshell
      // appshell source
   README.md
   ...
```

Open a command prompt on this directory and run `make_symlinks.bat`. This will create symbolic links to several folders in the `deps/cef` directory.

Open appshell.sln in Visual Studio. NOTE: If you are using Visual Studio Express, you may get warnings that say some of the projects couldn't be loaded. These can be ignored.

####Generating Projects
This is only required if you are changing the project files. **NOTE:** Don't change the Visual Studio project files directly. Any changes should be done to the .gyp files, and new Visual Studio projects should be generated.

* Add a <code>CHROMIUM\_SRC\_PATH</code> environment variable that points to your chromium 'src' folder (without a final '/').
* Open a command prompt on this directory and run <code>make\_appshell\_project.bat</code>

### Linux

Not available yet. Please let us know if you'd like to help with the Linux version.
