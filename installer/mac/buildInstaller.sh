#!/bin/bash 

# config
releaseName="Brackets Sprint 13"
format="bzip2"
encryption="none"
tmpLayout="./dropDmgConfig/layouts/tempLayout"
appName=$releaseName".app"
tempDir="tempBuild"

# rename app and copy to tempBuild director
rm -rf $tempDir
mkdir $tempDir
cp -r ./staging/Brackets.app/ "$tempDir/$appName"

# create disk layout
rm -rf $tempLayoutDir
cp -r ./dropDmgConfig/layouts/bracketsLayout/ $tmpLayout

# Replace APPLICATION_NAME in Info.plist with $releaseName.app
grep -rl APPLICATION_NAME $tmpLayout/Info.plist | xargs sed -i -e "s/APPLICATION_NAME/$releaseName.app/g"


# build the DMG
echo "building DMG..."
dropdmg ./$tempDir --format $format --encryption $encryption --layout-folder $tmpLayout --volume-name "$releaseName" --base-name "$releaseName"

# clean up
rm -rf $tempDir
rm -rf $tmpLayout