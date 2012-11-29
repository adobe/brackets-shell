# SETUP
# - Install Visual Studio 2010 (Express is fine)
# - Setup source directories as specified in README.md
#   (copy CEF binary, run scripts/make_symlinks.sh, etc.)
# - Set BRACKETS_SRC environment variable, pointing to the
#   brackets source code (without trailing '/')
# - Optionally, set BRACKETS_APP_NAME environment variable with the 
#   name of the application. (This should match the app name built
#   by the gyp project, and need not match the final installed build name.)
# - Optionally, set BRACKETS_SHELL_BRANCH and BRACKETS_BRANCH
#   to the branches you want to build.

# Make sure BRACKETS_SRC environment variable is set
if [ "$BRACKETS_SRC" = "" ]; then
  echo "The BRACKETS_SRC environment variable must be set to the location of the Brackets source folder. Aborting."
  exit
fi

# Default the app name to "Brackets", but override with $BRACKETS_APP_NAME if set
if [ "$BRACKETS_APP_NAME" = "" ]; then
  export BRACKETS_APP_NAME="Brackets"
fi

# Default the branches to "master"
if [ "$BRACKETS_SHELL_BRANCH" = "" ]; then
  export BRACKETS_SHELL_BRANCH="master"
fi
if [ "$BRACKETS_BRANCH" = "" ]; then
  export BRACKETS_BRANCH="master"
fi

# Pull the latest code
curDir=`pwd`
cd "$BRACKETS_SRC"
git checkout "$BRACKETS_BRANCH"
git pull origin "$BRACKETS_BRANCH"
git submodule update --init --recursive
build_num=`git log --oneline | wc -l | tr -d ' '`
brackets_sha=`git log | head -1 | sed -e 's/commit \([0-9a-f]*$\)/\1/'`
cd $curDir
git checkout "$BRACKETS_SHELL_BRANCH"
git pull origin "$BRACKETS_SHELL_BRANCH"

# Clean and build the Visual Studio project
# Create a .bat file that initializes the Visual Studio command line vars
# and calls the MSBuild tool to clean/build the solution.
echo "call \"$VS100COMNTOOLS/vsvars32.bat\"
msbuild.exe appshell.sln /t:Clean /p:Platform=Win32 /p:Configuration=Release
msbuild.exe appshell.sln /t:Build /p:Platform=Win32 /p:Configuration=Release
exit" > temp_build.bat
cmd /k temp_build.bat
rm temp_build.bat

echo "cd installer/win
call stageForInstaller.bat
cd ../..
exit" > temp_staging.bat
cmd /k temp_staging.bat
rm temp_staging.bat
installer/win/setBuildNumber.sh

# TODO - BUILD INSTALLER
