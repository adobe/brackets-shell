#!/bin/bash

# grunt-contrib-copy doesn't preserve permissions
# https://github.com/gruntjs/grunt/issues/615
chmod 775 installer/linux/debian/usr/lib/brackets/Brackets

# set permissions on subdirectories
find installer/linux/debian -type d | xargs chmod 755

# delete old package
rm -f installer/linux/brackets.deb

dpkg-deb --build installer/linux/debian
mv installer/linux/debian.deb installer/linux/brackets.deb