#!/bin/bash
echo 'uninstalling brackets from your ~/.local directory'
echo 'run with -c to remove ~/.config/Brackets'

removeConfig=false

while getopts c opt; do
  case $opt in
    c)
      removeConfig=true
      ;;
  esac
done

shift $((OPTIND - 1))

if [ "$removeConfig" = true ]; then
    echo 'deleting ~/.config/Brackets'
    rm -rf ~/.config/Brackets
fi

# Safety first: only uninstall files we know about.

rm -f ~/.local/bin/brackets
rm -f ~/.local/share/applications/brackets.desktop

echo 'finished uninstall brackets from your ~/.local directory'
