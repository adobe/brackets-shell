#!/bin/bash 

# config
releaseName='Brackets preview 1'
format="bzip2"
encryption="none"
layoutFolder="./dropDmgConfig/layouts/preview1"
licenseFolder="./dropDmgConfig/eulas/fr_FR"
appName=$releaseName".app"
tempDir="tempBuild"

# rename app and copy to tempBuild director
rm -rf $tempDir
mkdir $tempDir
cp -r ./stagging/Brackets.app/ "$tempDir/$appName"


dropdmg ./$tempDir --format $format --encryption $encryption --layout-folder $layoutFolder --license-folder $licenseFolder --volume-name "Brackets preview 1" --base-name "Brackets preview 1"


#rm -rf $tempDir