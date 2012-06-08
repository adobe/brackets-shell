#!/bin/bash

# Check for CHROMIUM_SRC_PATH
if [ "$CHROMIUM_SRC_PATH" = "" ]; then
  echo "CHROMIUM_SRC_PATH environment variable is not set. Aborting."
  exit
fi

# Build the project
echo "Building xcode project."
$CHROMIUM_SRC_PATH/tools/gyp/gyp appshell.gyp -I $CHROMIUM_SRC_PATH/build/common.gypi --depth=$CHROMIUM_SRC_PATH/

# Fix up paths
sed 's:[^ ]*/src/build/mac/:tools/:' appshell.xcodeproj/project.pbxproj > tmp_pbxproj.txt
mv tmp_pbxproj.txt appshell.xcodeproj/project.pbxproj
