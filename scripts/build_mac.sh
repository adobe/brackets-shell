# SETUP
# - Install xcode
# - Install xcode command line tools 
# - Setup source directories as specified in README.md
#   (copy CEF binary, run scripts/make_symlinks.sh, etc.)
# - Set BRACKETS_SRC environment variable, pointing to the
#   brackets source code (without trailing '/')

# Set this to the branch you want to build from
branchName=master

# Pull the latest code
curDir=`pwd`
cd $BRACKETS_SRC
git checkout $branchName
git pull origin $branchName
cd $curDir
git checkout $branchName
git pull origin $branchName

# Clean and build the xcode project
xcodebuild -project appshell.xcodeproj -config Release clean
xcodebuild -project appshell.xcodeproj -config Release build

# Package www files
scripts/package_www_files.sh

