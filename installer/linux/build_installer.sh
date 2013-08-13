#!/bin/bash

# grunt-contrib-copy doesn't preserve permissions
# https://github.com/gruntjs/grunt/issues/615
chmod 775 debian/package-root/usr/lib/brackets/Brackets

# set permissions on subdirectories
find debian -type d -exec chmod 755 {} \;

# delete old package
rm -f brackets.deb

fakeroot dpkg-deb --build debian/package-root
mv debian/package-root.deb brackets.deb
