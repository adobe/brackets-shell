#!/bin/bash 

# config
releaseName="Brackets preview 1"
format="bzip2"
encryption="none"
layoutFolder="./dropDmgConfig/layouts/preview1"
appName=$releaseName".app"
tempDir="tempBuild"

# rename app and copy to tempBuild director
rm -rf $tempDir
mkdir $tempDir
cp -r ./staging/Brackets.app/ "$tempDir/$appName"


dropdmg ./$tempDir --format $format --encryption $encryption --layout-folder $layoutFolder --volume-name "Brackets preview 1" --base-name "Brackets preview 1"


#rm -rf $tempDir