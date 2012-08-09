# Make sure BRACKETS_WWW_SRC environment variable is set
if [ "$BRACKETS_WWW_SRC" = "" ]; then
  echo "BRACKETS_WWW_SRC environment variable is not set. Aborting."
  exit
fi

# Remove existing www directory contents
rm -rf xcodebuild/Release/Brackets.app/Contents/www/*
rmdir xcodebuild/Release/Brackets.app/Contents/www

# Make the Brackets.app/Contents/www directory
mkdir xcodebuild/Release/Brackets.app/Contents/www

# Copy the source 
cp -pR $BRACKETS_WWW_SRC/* xcodebuild/Release/Brackets.app/Contents/www
