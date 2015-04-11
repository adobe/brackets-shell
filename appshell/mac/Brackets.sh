#!/bin/bash

TARGET_FILE=$0
BRACKETS_APP_NAME="Brackets.app"

# These are to check for any instances of there are thete
BASE_APP_NAME="Brackets"
BASE_APP_EXT=".app"

TARGET_PATH="$(readlink "$TARGET_FILE")"
TARGET_DIR="$(dirname "$TARGET_PATH")"

FINAL_APP_PATH=TARGET_DIR
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
    FINAL_APP_PATH=TARGET_DIR
else
    FINAL_APP_PATH="";
fi

# Give preference to launching the script's enclosing app
if [ -x "$FINAL_APP_PATH" ]; then
    open -a "$FINAL_APP_PATH" "$@"
elif [ -x "/Applications/$BRACKETS_APP_NAME" ]; then
    open -a /Applications/$BRACKETS_APP_NAME "$@"
elif [ -x "$HOME/Applications/$BRACKETS_APP_NAME" ]; then
    open -a $HOME/Applications/$BRACKETS_APP_NAME "$@"
else
    open -a Brackets "$@"
fi
