#!/bin/bash

# SETUP - MAC
# - Install xcode
# - Install xcode command line tools 
# - Install DropDMG, including the dropdmg command line tool
# SETUP - WIN
# - Install Visual Studio 2010 (Express is fine)
# SETUP - COMMON
# - Run scripts/setup.sh from a Bash prompt (Terminal on Mac,
#   GitBash on Windows).
# - Set BRACKETS_SRC environment variable, pointing to the
#   brackets source code (without trailing '/')
# - Optionally, set BRACKETS_APP_NAME environment variable with the 
#   name of the application. (This should match the app name built
#   by the gyp project, and need not match the final installed build name.)
# - Optionally, set BRACKETS_SHELL_BRANCH and BRACKETS_BRANCH
#   to the branches you want to build. You can set either of these to 
#   NO_FETCH to skip the checkout and fetching for that repo.

# Make sure BRACKETS_SRC environment variable is set
if [ "$BRACKETS_SRC" = "" ]; then
    echo "The BRACKETS_SRC environment variable must be set to the location of the Brackets source folder. Aborting."
    exit
fi

# Default the app name to "Brackets", but override with $BRACKETS_APP_NAME if set
if [ "$BRACKETS_APP_NAME" = "" ]; then
    export BRACKETS_APP_NAME="Brackets"
fi

# Default the branches to "master"
# You can set either branch name to "NO_FETCH" to skip the checkout and fetching for that repo
if [ "$BRACKETS_SHELL_BRANCH" = "" ]; then
    export BRACKETS_SHELL_BRANCH="master"
fi
if [ "$BRACKETS_BRANCH" = "" ]; then
    export BRACKETS_BRANCH="master"
fi

# Pull the latest brackets code
pushd "$BRACKETS_SRC"
if [ "$BRACKETS_BRANCH" != "NO_FETCH" ]; then
    git checkout "$BRACKETS_BRANCH"
    git pull origin "$BRACKETS_BRANCH"
    git submodule update --init --recursive
else
    echo "Skipping fetch for brackets repo"
fi
# Calculate the build number if it's not already set.
if [ "$BRACKETS_BUILD_NUM" = "" ]; then
    export BRACKETS_BUILD_NUM=`git log --oneline | wc -l | tr -d ' '`
fi
brackets_sha=`git log | head -1 | sed -e 's/commit \([0-9a-f]*$\)/\1/'`
brackets_branch_name=`git status | head -1 | sed -e 's/# On branch \(.*\)/\1/'`
if [ "$brackets_branch_name" = "# Not currently on any branch." ]; then
    brackets_branch_name="SHA"
fi
popd

# Pull the latest brackets-shell code
if [ "$BRACKETS_SHELL_BRANCH" != "NO_FETCH" ]; then
    git checkout "$BRACKETS_SHELL_BRANCH"
    git pull origin "$BRACKETS_SHELL_BRANCH"
else
    echo "Skipping fetch for brackets-shell repo"
fi

# Rebuild project files after potentially checking out new code
scripts/make_appshell_project.sh

os=${OSTYPE//[0-9.]/}

if [ "$os" = "darwin" ]; then # Building on mac
    # Clean and build the xcode project
    xcodebuild -project appshell.xcodeproj -config Release clean
    xcodebuild -project appshell.xcodeproj -config Release build
    
    # Remove existing staging dir
    if [ -d installer/mac/staging ]; then
      rm -rf installer/mac/staging
    fi
    
    mkdir installer/mac/staging
    
    # Copy to installer staging folder
    cp -R "xcodebuild/Release/${BRACKETS_APP_NAME}.app" installer/mac/staging/
     
    # Package www files
    scripts/package_www_files.sh

    packageLocation="installer/mac/staging/${BRACKETS_APP_NAME}.app/Contents/www"

elif [ "$os" = "msys" ]; then # Building on Windows

    # Clean and build the Visual Studio project
    cmd.exe /c "scripts\build_projects.bat"

    # Stage files for installer
    cmd.exe /c "scripts\stage_for_installer.bat"

    packageLocation="installer/win/staging/www"

else 
    echo "Unknown platform \"$os\". I don't know how to build this."
    exit;
fi


# Set the build number, branch and sha on the staged build
cat "$packageLocation/config.json" \
|   sed "s:\(\"version\"[^\"]*\"[0-9.]*-\)\([0-9*]\)\(\"\):\1$BRACKETS_BUILD_NUM\3:" \
|   sed "s:\(\"branch\"[^\"]*\"\)\([^\"]*\)\(\"\):\1$brackets_branch_name\3:" \
|   sed "s:\(\"SHA\"[^\"]*\"\)\([^\"]*\)\(\"\):\1$brackets_sha\3:" \
> tmp_package_json.txt
mv tmp_package_json.txt "$packageLocation/config.json"

# Build the installer
if [ "$os" = "darwin" ]; then # Build mac installer
    cd installer/mac
    ./buildInstaller.sh

# TODO: Build windows installer
fi
