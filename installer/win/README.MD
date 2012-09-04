## Overview
This readme describes how the .msi installer is created for Brackets on Windows.


## System setup
_**Note:**_ currently, the scripts will only run from the build machine. They make several assumptions about paths of specific
components that must be present. TODO: More details comining soon.


## To rev the Brackets sprint/build number
1. Open brackets-win-install-build.xml and change `product.sprint.number`


## To build the Brackets installer
1. Sync both the brackets and brackets-shell repos
    * Be sure to update submodules in brackets
    * Double check that your git status is clean
2. Build brackets-shell
    * This cannot be done on the build machine yet, so generate a copy elsewhere and copy it into brackets-shell\Release
      on the build machine
3. Follow instructions in brackets-shell\installer\win\stageForInstaller.bat to run it
    * This will generate brackets-shell\installer\win\staging
4. Update build numbers as needed (see previous section)
5. Run `ant -f brackets-win-install-build.xml build.mul` in installer\win\staging
    * Output is "Brackets Sprint _NN_.msi"
    * This creates lots of intermediate files, too. To clean them up, run the `clean.temp` target
    * Note: you'll see four warning messages saying "The file ... is not a Font, and its version is not a companion file
      reference." These are safe to ignore - see issue #1471.
      (Just make sure the *end* of each chunk of output doesn't list any errors).
