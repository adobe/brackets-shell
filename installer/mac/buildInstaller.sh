#!/bin/bash 

# config
releaseName="Brackets Sprint 13"
format="bzip2"
encryption="none"
layoutFolder="./dropDmgConfig/layouts/bracketsLayout"
appName=$releaseName".app"
tempDir="tempBuild"

# rename app and copy to tempBuild director
rm -rf $tempDir
mkdir $tempDir
cp -r ./staging/Brackets.app/ "$tempDir/$appName"


# build the DMG
echo "building DMG..."
dropdmg ./$tempDir --format $format --encryption $encryption --layout-folder $layoutFolder --volume-name "$releaseName" --base-name "$releaseName"

# clean up
rm -rf $tempDir