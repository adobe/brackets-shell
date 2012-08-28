## Overview
This readme describes how the .msi installer is created for Brackets on Windows.

_**Note:**_ currently, the scripts will only run from a certain hardcoded location on the build machine. The
scripts are only checked into git for backup. Any changes made to the scripts in brackets-shell\installer\win
must be manually copied to F:\stage\Brackets\InstallerFiles to take effect.


## To rev the Brackets sprint/build number
_**TODO: This is currently hardcoded in multiple places in brackets-win-install-build.xml and Brackets.wxs**_


## To build the Brackets installer
1. Build brackets-shell
    * This cannot be done on the build machine yet, so generate a copy elsewhere and copy it into brackets-shell\Release
      on the build machine
2. Follow instructions in brackets-shell\installer\win\stageForInstaller.bat to run it
    * This will generate brackets-shell\installer\win\staging
3. Copy contents of brackets-shell\installer\win\staging to F:\stage\Brackets\BinarySource
4. Update build numbers as needed (see previous section)
5. Go to F:\stage\Brackets\InstallerFiles
6. Run `ant -f brackets-win-install-build.xml -Dofficial.build.number=1 build.mul`
    * Output is in F:\stage\Brackets\InstallerFiles\Brackets Sprint NN.msi
    * Note: you'll see four warning messages saying "The file ... is not a Font, and its version is not a companion file
      reference." These are safe to ignore - see issue #1471.
      (Just make sure the *end* of each chunk of output doesn't list any errors).