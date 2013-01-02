## Overview
This readme describes how the .msi installer is created for Brackets on Windows.


## System setup
Normally, the installer scripts are run from a specific pre-set-up build machine. These are instructions for setting up a _new_ build machine:

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
10. `git clone` the brackets and brackets-shell Git repos
11. Set `BRACKETS_SRC` to the root folder of the brackets repo


## To rev the Brackets sprint number
1. Open brackets-win-install-build.xml and change `product.sprint.number`
2. Open brackets-shell\installer\mac\buildInstaller.sh and change `releaseName`
3. Open Brackets.wxs and replace the `StartMenuShortcut` GUID property with a newly generated GUID
4. Open brackets-shell\appshell\version.rc and change `FILEVERSION` and `"FileVersion"`
5. Open brackets-shell\appshell\mac\Info.plist and change `CFBundleVersion`
6. Open brackets\src\package.json and change `"version"`

## To build the Brackets installer
1. Sync both the brackets and brackets-shell repos
    * Be sure to update submodules in brackets
    * Double check that your git status is clean
2. Update sprint number as needed (see previous section)
3. Build brackets-shell
    * This cannot be done on the build machine yet, so generate a copy elsewhere and copy it into brackets-shell\Release
      on the build machine
4. Follow instructions in brackets-shell\installer\win\stageForInstaller.bat to run it
    * This will generate brackets-shell\installer\win\staging
5. Follow instructions in brackets-shell\installer\win\setBuildNumber.sh to run it (in the Git Bash window)
    * This will inject the build number, branch, and SHA into the staged package.json file
6. Run `ant -f brackets-win-install-build.xml` in installer\win\staging
    * Output is "Brackets Sprint _NN_.msi"
    * This creates lots of intermediate files, too. To clean them up, run the `clean.temp` target
    * Note: you'll see two sets of five warning messages saying "The file ... is not a Font, and its version is not a
      companion file reference." These are safe to ignore - see issue #1471.
      (Just make sure the *end* of each chunk of output doesn't list any errors).
