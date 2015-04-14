#!/bin/bash

# The actual location of the symlink is passed
# in $0. We first get the correct target from the
# symlink and then go about finding the enclosing
# app. Once a .app is found, we plainly pass the
# location of the app to open command with a -a
# as an argument. Note that readlink works differntly
# on mac.

TARGET_FILE=$0
BRACKETS_APP_NAME="Brackets.app"

TARGET_PATH="$(readlink "$TARGET_FILE")"
TARGET_DIR="$(dirname "$TARGET_PATH")"

FINAL_APP_PATH="$TARGET_DIR"
if [ ! -z "$TARGET_PATH" -a ! -z "$TARGET_DIR" ]; then
    BASE_PATH_NAME="$(basename "$TARGET_DIR")"

    # Now that we have found the location go about
    # trying to find the enclosing app path.
    while [[ "$BASE_PATH_NAME" != *.app ]]
    do
        TARGET_DIR="$(dirname "$TARGET_DIR")"
        BASE_PATH_NAME="$(basename "$TARGET_DIR")"
        if [ -z "$TARGET_DIR" -o "$TARGET_DIR" == "/" ]; then
            TARGET_DIR="";
            break;
        fi
    done
    FINAL_APP_PATH="$TARGET_DIR"
else
    FINAL_APP_PATH="";
fi

# Give preference to launching the script's enclosing app
# followed by the one in HOME/Applications. Last one
# would be checked for global /Applications

if [ -x "$FINAL_APP_PATH" ]; then
    open -a "$FINAL_APP_PATH" "$@"
elif [ -x "$HOME/Applications/$BRACKETS_APP_NAME" ]; then
    open -a $HOME/Applications/$BRACKETS_APP_NAME "$@"
elif [ -x "/Applications/$BRACKETS_APP_NAME" ]; then
    open -a /Applications/$BRACKETS_APP_NAME "$@"
else
    open -a Brackets "$@"
fi
