#!/bin/bash

# Give preference to launching the one in /Applications folder
if [ -x "/Applications/Brackets.app" ]; then
    open -a /Applications/Brackets.app "$@"
else
    open -a Brackets "$@"
fi
