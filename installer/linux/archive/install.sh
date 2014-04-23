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

# Run xdg-desktop-menu to register the .desktop file.
# This is for Unity's benefit: Gnome 3 and KDE 4 both watch ~/.local/share/applications and install .desktop files automatically.
# Copy-pasta this straight from the .deb's postinst script.
XDG_DESKTOP_MENU="`which xdg-desktop-menu 2> /dev/null`"
if [ ! -x "$XDG_DESKTOP_MENU" ]; then
  echo "Error: Could not find xdg-desktop-menu" >&2
  exit 1
fi
"$XDG_DESKTOP_MENU" install ~/.local/share/applications/brackets.desktop --novendor

# Symlink brackets executable to our current location... where-ever that is.
rm -f ~/.local/bin/brackets
ln -s $execPath ~/.local/bin/brackets

echo 'installed brackets to ~/.local'
