# Remove existing links
rm -f Debug include libcef_dll Release Resources tools

# Make new links
ln -s deps/cef/Debug/ Debug
ln -s deps/cef/include/ include
ln -s deps/cef/libcef_dll/ libcef_dll
ln -s deps/cef/Release/ Release
ln -s deps/cef/Resources/ Resources
ln -s deps/cef/tools/ tools

# Make sure scripts in deps/cef/tools are executable
chmod u+x deps/cef/tools/*
