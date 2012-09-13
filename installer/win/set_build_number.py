# This script sets the build number, branch name and SHA in the 
# staged package.json file.
#
# This script is called by the stageForInstaller script.
#
# The following environment variables MUST be set:
#  STAGED_SRC - path to the www directory in the staging folder
#  BUILD_NUM - Build number to write into package.json file
#  BUILD_BRANCH - Name of branch to write into package.json file
#  BUILD_SHA - SHA to write into package.json file
#
# If any of these variables are empty, this script writes a warning message
# and exits

import os
import re
import shutil

# Check for environment variables
staged_src = os.getenv("STAGED_SRC")
build_num = os.getenv("BUILD_NUM")
build_branch = os.getenv("BUILD_BRANCH")
build_sha = os.getenv("BUILD_SHA")

if (not staged_src) or (not build_num) or (not build_branch) or (not build_sha):
    print("One or more environment variables was not set.")
    print("Please make sure STAGED_SRC, BUILD_NUM, BUILD_BRANCH, and BUILD_SHA are all set.")
    exit()

staged_filename = staged_src + "/package.json"
tmp_filename = "build_number.tmp"

tmp_file = open(tmp_filename , "w");
pkg_file = open(staged_filename , "r")

for line in pkg_file:
    line = re.sub(r'("version"[^"]*"[0-9.]*-)([0-9]*)(")', r'\g<1>'+build_num+'\g<3>', line);
    line = re.sub(r'("branch"[^"]*")([^"]*)(")', r'\g<1>'+build_branch+'\g<3>', line);
    line = re.sub(r'("SHA"[^"]*")([^"]*)(")', r'\g<1>'+build_sha+'\g<3>', line);
    tmp_file.write(line);

pkg_file.close();
tmp_file.close();

# Finally, move the tmp file back to staging
shutil.move(tmp_filename, staged_filename)
