#!/bin/bash

# Make sure BRACKETS_SRC environment variable is set
if [ "$BRACKETS_SRC" = "" ]; then
  echo "The BRACKETS_SRC environment variable must be set to the location of the Brackets source folder. Aborting."
  exit
fi

# Remove existing www directory contents
if [ -d "installer/mac/staging/${BRACKETS_APP_NAME}.app/Contents/www" ]; then
  rm -rf "installer/mac/staging/${BRACKETS_APP_NAME}.app/Contents/www"
fi

# Remove existing samples directory contents
if [ -d "installer/mac/staging/${BRACKETS_APP_NAME}.app/Contents/samples" ]; then
  rm -rf "installer/mac/staging/${BRACKETS_APP_NAME}.app/Contents/samples"
fi

# Make the ${BRACKETS_APP_NAME}.app/Contents/www directory
mkdir "installer/mac/staging/${BRACKETS_APP_NAME}.app/Contents/www"

# Copy the source 
cp -pR "${BRACKETS_SRC}"/src/* "installer/mac/staging/${BRACKETS_APP_NAME}.app/Contents/www"

# Make the ${BRACKETS_APP_NAME}.app/Contents/samples directory
mkdir "installer/mac/staging/${BRACKETS_APP_NAME}.app/Contents/samples"

# Copy the source 
cp -pR "${BRACKETS_SRC}"/samples/* "installer/mac/staging/${BRACKETS_APP_NAME}.app/Contents/samples"
