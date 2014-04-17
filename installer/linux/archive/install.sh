#!/bin/bash
echo 'installing brackets to your ~/.local directory'

curDir=$(pwd)

mkdir -p ~/.local/share/applications
mkdir -p ~/.local/bin

# Setup our brackets.desktop file.
cp brackets.desktop temp.desktop
execPath="$curDir/brackets"
iconPath="$curDir/brackets.svg"

sed -i 's|Exec=brackets|Exec='$execPath'|g' temp.desktop
sed -i 's|Icon=brackets|Icon='$iconPath'|g' temp.desktop
cp temp.desktop ~/.local/share/applications/brackets.desktop
rm temp.desktop

# Symlink brackets executable to our current location... where-ever that is.
rm -f ~/.local/bin/brackets
ln -s $execPath ~/.local/bin/brackets

echo 'installed brackets to ~/.local'
