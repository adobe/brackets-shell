# NOTE: THIS SCRIPT WILL BE GOING AWAY SOON. IT HAS BEEN SUPERSEDED BY 
# /scripts/build.sh
#
# Injects the build number, branch, and SHA into the staged package.json file.
#
# INSTRUCTIONS FOR RUNNING
# ------------------------
# 1. This script MUST be run from the Git Bash shell
#    This script should be run *after* running stageForInstaller.bat
# 2. Set the BRACKETS_SRC environment variable to point to the location
#    of the brackets source folder. 
#    e.g. export BRACKETS_SRC=/c/Users/glenn/dev/brackets
# 3. cd to the root of the brackets-shell repo
# 4. Run installer/win/setBuildNumber.sh


# Make sure BRACKETS_SRC environment variable is set
if [ "$BRACKETS_SRC" = "" ]; then
  echo "The BRACKETS_SRC environment variable must be set to the location of the Brackets source folder. Aborting."
  exit
fi

# Make sure the staged package.json file exists
if [ ! -f "installer/win/staging/www/package.json" ]; then
	echo "I can't find \"installer/win/staging/www/package.json\""
	echo "This script must be run from the root brackets-shell directory"
	exit
fi

curDir=`pwd`
cd "$BRACKETS_SRC"
build_num=`git log --oneline | wc -l | tr -d ' '`
branch_name=`git status | head -1 | sed -e 's/# On branch \(.*\)/\1/'`
brackets_sha=`git log | head -1 | sed -e 's/commit \([0-9a-f]*$\)/\1/'`
cd $curDir

# Set the build number, branch and sha on the staged build
cat "installer/win/staging/www/package.json" \
|   sed "s:\(\"version\"[^\"]*\"[0-9.]*-\)\([0-9*]\)\(\"\):\1$build_num\3:" \
|   sed "s:\(\"branch\"[^\"]*\"\)\([^\"]*\)\(\"\):\1$branch_name\3:" \
|   sed "s:\(\"SHA\"[^\"]*\"\)\([^\"]*\)\(\"\):\1$brackets_sha\3:" \
> tmp_package_json.txt
mv tmp_package_json.txt "installer/win/staging/www/package.json"
