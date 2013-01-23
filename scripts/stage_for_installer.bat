:: Batch file that simply calls installer/win/stageForInstaller.
:: This file is used by build.sh

cd installer/win
call stageForInstaller.bat
cd ../..
exit /b
