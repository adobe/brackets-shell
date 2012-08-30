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

# Remove existing samples directory contents
if [ -d xcodebuild/Release/Brackets.app/Contents/samples ]; then
  rm -rf xcodebuild/Release/Brackets.app/Contents/samples/*
  rmdir xcodebuild/Release/Brackets.app/Contents/samples
fi

# Make the Brackets.app/Contents/www directory
mkdir xcodebuild/Release/Brackets.app/Contents/www

# Copy the source 
cp -pR $BRACKETS_SRC/src/* xcodebuild/Release/Brackets.app/Contents/www

# Make the Brackets.app/Contents/samples directory
mkdir xcodebuild/Release/Brackets.app/Contents/samples

# Copy the source 
cp -pR $BRACKETS_SRC/samples/* xcodebuild/Release/Brackets.app/Contents/samples
