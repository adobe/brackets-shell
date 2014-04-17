#!/bin/bash
echo 'uninstalling brackets from your ~/.local directory'

# Safety first: only uninstall files we know about.

rm -f ~/.local/bin/brackets
rm -f ~/.local/share/applications/brackets.desktop

echo 'finished uninstall brackets from your ~/.local directory'
