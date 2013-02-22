#!/bin/bash

os=${OSTYPE//[0-9.]/}

# Get the full path of this script
if [[ ${0} == /* ]]; then
  full_path="$0"
else
  full_path="${PWD}/${0#./}"
fi

# Remove /scripts/download_deps.sh to get the root directory
root_dir=${full_path%/*/*}

#### CEF

cefVersion="3.1180.823"
if [ "$os" = "darwin" ]; then # Building on mac
    zipName="cef_binary_""$cefVersion""_macosx"
elif [ "$os" = "msys" ]; then # Building on win
    zipName="cef_binary_""$cefVersion""_windows"
fi

# See if we already have the correct version
if [ -f "$root_dir/deps/cef/$zipName.txt" ]; then
    echo "You already have the correct version of CEF downloaded."
    
else
    # nuke the old folder
    rm -rf "$root_dir/deps/cef"
    
    echo "Downloading CEF binary distribution"
    curl -# "http://chromiumembedded.googlecode.com/files/$zipName.zip" > tmp.zip
    
    echo "Extracting ZIP file"
    unzip -q tmp.zip -d "$root_dir/deps"
    rm tmp.zip
    
    # Rename
    mv "$root_dir/deps/$zipName" "$root_dir/deps/cef"
    
    # Write a text file with the zip name 
    echo "$zipName" > "$root_dir/deps/cef/$zipName.txt"
    
    # Make the symlinks
    pushd "$root_dir"
    if [ "$os" = "msys" ]; then
        # Call batch file to make directory junctions
        cmd.exe /c "scripts\make_symlinks.bat"
    else
        rm Debug include libcef_dll Release Resources tools
        
        # Make symlinks to deps/cef directories
        ln -s deps/cef/Debug/ Debug
        ln -s deps/cef/include/ include
        ln -s deps/cef/libcef_dll/ libcef_dll
        ln -s deps/cef/Release/ Release
        ln -s deps/cef/Resources/ Resources
        ln -s deps/cef/tools/ tools

        # Make sure scripts in deps/cef/tools are executable
        chmod u+x deps/cef/tools/*
    fi
    popd
fi

#### NODE

nodeVersion="0.8.20"
npmVersion="1.2.11"

# See if we already have the correct version
if [ -f "$root_dir/deps/node/version-$nodeVersion.txt" ]; then
    echo "You already have the correct version of Node downloaded."

else
    # nuke the old folder
    rm -rf "$root_dir/deps/node"
    
    if [ "$os" = "darwin" ]; then # Building on mac
        echo "Downloading Node"
        curl -# "http://nodejs.org/dist/v$nodeVersion/node-v$nodeVersion-darwin-x86.tar.gz" > tmp.tar.gz
        echo "Extracting tar file"
        tar -xzf tmp.tar.gz
        rm tmp.tar.gz
        mv "node-v$nodeVersion-darwin-x86" "$root_dir/deps/node"

        # Create a copy of the "node" binary as "Brackets-node". We need one named "node"
        # for npm to function properly, but we want to call the executable "Brackets-node"
        # in the final binary. Due to gyp's limited nature, we can't (easily) do this rename
        # as part of the build process.
        cp "$root_dir/deps/node/bin/node" "$root_dir/deps/node/bin/Brackets-node"

        echo "$nodeVersion" > "$root_dir/deps/node/version-$nodeVersion.txt"
    elif [ "$os" = "msys" ]; then # Building on win
        echo "Downloading Node"
        mkdir "$root_dir/deps/node"
        curl -# "http://nodejs.org/dist/v$nodeVersion/node.exe" -o "$root_dir/deps/node/node.exe"

        # Create a copy of the "node.exe" binary as "Brackets-node.exe". We do this copying
        # for the same reason as above in the mac case.
        cp "$root_dir/deps/node/node.exe" "$root_dir/deps/node/Brackets-node.exe"

        echo "Downloading NPM"
        curl -# "http://nodejs.org/dist/npm/npm-$npmVersion.zip" > tmp.zip
        
        echo "Extracting ZIP file"
        unzip -q tmp.zip -d "$root_dir/deps/node"
        rm tmp.zip
        
        echo "$nodeVersion" > "$root_dir/deps/node/version-$nodeVersion.txt"
    fi

fi

echo "Node setup complete!"
