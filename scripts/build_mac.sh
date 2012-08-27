# SETUP
# - Install xcode
# - Install xcode command line tools 
# - Install DropDMG, including the dropdmg command line tool
# - Setup source directories as specified in README.md
#   (copy CEF binary, run scripts/make_symlinks.sh, etc.)
# - Set BRACKETS_SRC environment variable, pointing to the
#   brackets source code (without trailing '/')

# Make sure BRACKETS_SRC environment variable is set
if [ "$BRACKETS_SRC" = "" ]; then
  echo "BRACKETS_SRC environment variable is not set. Aborting."
  exit
fi

# Set this to the branch you want to build from
branchName=master

# Pull the latest code
curDir=`pwd`
cd $BRACKETS_SRC
git checkout $branchName
git pull origin $branchName
git submodule update --init --recursive
cd $curDir
git checkout $branchName
git pull origin $branchName

# Clean and build the xcode project
xcodebuild -project appshell.xcodeproj -config Release clean
xcodebuild -project appshell.xcodeproj -config Release build

# Package www files
scripts/package_www_files.sh

# Remove existing staging dir
if [ -d installer/mac/staging ]; then
  rm -rf installer/mac/staging/*
  rmdir installer/mac/staging
fi

mkdir installer/mac/staging

# Copy to installer staging folder
cp -R xcodebuild/Release/Brackets.app installer/mac/staging/

# Build the installer
cd installer/mac
./buildInstaller.sh