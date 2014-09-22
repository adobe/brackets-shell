## Overview
This readme describes how the .msi installer is created for Brackets on Windows.

## To build the Brackets installer (after completing System Setup below)
1. grunt installer

## System Setup
Normally, the installer scripts are run via Jenkins on our build machine. These are instructions for setting up a _new_ build machine:

1. Download Ant; unzip it into a path (ideally a path with no spaces -- see below)
2. Download & install a JDK (the full SDK, _not_ just a JRE)
3. Set `ANT_HOME` to the path to Ant. Must not contain spaces -- if target dir does have spaces, use the DOS-style path mapping (e.g. `c:\progra~1\...`)
4. Set `JAVA_HOME` to the path to Java. Must not contain spaces -- same as above.
5. Add `%ANT_HOME%\bin` to the PATH
6. Download & install Wix 3.5
7. Get the MsiTran.exe tool:
    * Option A: If you have Visual Studio installed, look in C:\Program Files\Microsoft SDKs\Windows\v??\bin to locate it
    * Option B: Follow the [instructions here](http://www.geektieguy.com/2010/03/13/create-a-multi-lingual-multi-language-msi-using-wix-and-custom-build-scripts/) to
      download the Windows SDK ISO and extract just the bit you need
    * Option C: Download the [full Windows SDK online installer](http://www.microsoft.com/en-us/download/details.aspx?id=3138). On
       the "Installation Options" screen, uncheck everything but "Win32 Development Tools"
    * Copy it to _(wix-install-dir)_\bin
8. Get the WiSubStg.vbs and WiLangId.vbs scripts:
    * Option A: Follow the [instructions here](http://www.geektieguy.com/2010/03/13/create-a-multi-lingual-multi-language-msi-using-wix-and-custom-build-scripts/) to
      download the Windows SDK ISO and extract just the bit you need
    * Option B: Download the [full Windows SDK online installer](http://www.microsoft.com/en-us/download/details.aspx?id=3138). On
      the "Installation Options" screen, uncheck everything but "Samples"
    * Copy these two files to _(wix-install-dir)_\bin
9. You may need to locally edit `wix.base` in brackets-win-install-build.xml if the Wix install folder doesn't match what's hardcoded there

## Update Release Number with Grunt
To rev the Brackets release number with Grunt, both brackets-shell and brackets repositories must be updated. For the initial Grunt setup, see: https://github.com/adobe/brackets/wiki/Grunt-Setup.

brackets       > grunt set-release --release=NN
brackets-shell > grunt set-release --release=NN

## To rev the Brackets release number manually
1. Open brackets-win-install-build.xml and change `product.release.number.minor`, and, for a major release, `product.release.number.major` as well
2. Open brackets-shell\installer\mac\buildInstaller.sh and change `dmgName`
3. Open Brackets.wxs and replace the `StartMenuShortcut` GUID property with a newly generated GUID
4. Open brackets-shell\appshell\version.rc and change `FILEVERSION` and `"FileVersion"`
5. Open brackets-shell\appshell\mac\Info.plist and change `CFBundleVersion` and `CFBundleShortVersionString`
6. Open brackets\src\package.json and change `"version"`
