#!/bin/bash

# config
releaseName="Brackets"
version="1.5"
dmgName="${releaseName} Release ${version}"
format="bzip2"
encryption="none"
tmpLayout="./dropDmgConfig/layouts/tempLayout"
appName="${releaseName}.app"
tempDir="tempBuild"

# rename app and copy to tempBuild director
rm -rf $tempDir
mkdir $tempDir
cp -r "./staging/${BRACKETS_APP_NAME}.app/" "$tempDir/$appName"

# create symlink to Applications folder in staging area
# with a single space as the name so it doesn't show an unlocalized name
ln -s /Applications "$tempDir/ "

# copy volume icon to staging area if one exists
customIcon=""
if [ -f ./assets/VolumeIcon.icns ]; then
  cp ./assets/VolumeIcon.icns "$tempDir/.VolumeIcon.icns"
  customIcon="--custom-icon"
fi

# if license folder exists, use it
if [ -d ./dropDmgConfig/licenses/bracketsLicense ]; then
  customLicense="--license-folder ./dropDmgConfig/licenses/bracketsLicense"
fi

# create disk layout
rm -rf $tempLayoutDir
cp -r ./dropDmgConfig/layouts/bracketsLayout/ "$tmpLayout"

# build the DMG
echo "building DMG..."
dropdmg ./$tempDir --format $format --encryption $encryption $customIcon --layout-folder "$tmpLayout" $customLicense --volume-name "$dmgName" --base-name "$dmgName"

# clean up
rm -rf $tempDir
rm -rf $tmpLayout
