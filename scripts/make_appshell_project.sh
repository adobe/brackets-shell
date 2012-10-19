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

sed 's:SYMROOT = \([^\;]*\);:SYMROOT = xcodebuild;:' appshell.xcodeproj/project.pbxproj > tmp_pbxproj.txt
mv tmp_pbxproj.txt appshell.xcodeproj/project.pbxproj

# Included chromium .gypi files set SDK to 10.7 in a couple places. Replace with empty string
# to use default SDK
sed 's:macosx10.7:"":' appshell.xcodeproj/project.pbxproj > tmp_pbxproj.txt
mv tmp_pbxproj.txt appshell.xcodeproj/project.pbxproj

# XCode is complaining about the "All" target not being optimized for multi-resolution images.
# I couldn't find a clean way to fix that in the .gyp file, so it is hard-coded here.
sed 's:PRODUCT_NAME = All;:PRODUCT_NAME = All; COMBINE_HIDPI_IMAGES = YES;:' appshell.xcodeproj/project.pbxproj > tmp_pbxproj.txt
mv tmp_pbxproj.txt appshell.xcodeproj/project.pbxproj
