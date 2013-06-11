#~/bin/bash

# clean old files
rm -rf installer/linux/staging
mkdir -p installer/linux/staging

# copy binaries to staging
cp -r out/Release/lib installer/linux/staging
cp -r out/Release/locales installer/linux/staging
cp -r out/Release/appshell.ico installer/linux/staging
cp -r out/Release/Brackets installer/linux/staging
cp -r out/Release/cef.pak installer/linux/staging
cp -r out/Release/devtools_resources.pak installer/linux/staging

# copy www and samples files to staging
cp -r ../brackets/src installer/linux/staging/www
cp -r ../brackets/samples installer/linux/staging/samples
