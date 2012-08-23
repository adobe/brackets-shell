#!/bin/bash

# Make sure BRACKETS_SRC environment variable is set
if [ "$BRACKETS_SRC" = "" ]; then
  echo "BRACKETS_SRC environment variable is not set. Aborting."
  exit
fi

# Remove existing www directory contents
if [ -d xcodebuild/Release/Brackets.app/Contents/www ]; then
  rm -rf xcodebuild/Release/Brackets.app/Contents/www/*
  rmdir xcodebuild/Release/Brackets.app/Contents/www
fi

# Make the Brackets.app/Contents/www directory
mkdir xcodebuild/Release/Brackets.app/Contents/www

# Copy the source 
cp -pR $BRACKETS_SRC/src/* xcodebuild/Release/Brackets.app/Contents/www
