Setup
=====

Run ./install to install the .desktop file and brackets command line tool to your ~/.local directory.
Run ./uninstall to remove these files.

Requires libudev.so.0. install.sh will attempt to symlink this file automatically if it exists.

If you see a libudev-related error message, you may need to do this:

     # 64-bit Fedora system.
     ln -s /usr/lib64/libudev.so.1 /usr/lib64/libudev.so.0
     # 32-bit Fedora system
     ln -s /usr/lib/libudev.so.1 /usr/lib/libudev.so.0

If you are unsure of your library directory, you can run the following command and examine its output to find the right path:

     ldd Brackets
     # Sample output:
     # libpangocairo-1.0.so.0 => /lib64/libpangocairo-1.0.so.0 (0x00007f482aed6000)
     # libatk-1.0.so.0 => /lib64/libatk-1.0.so.0 (0x00007f482acb3000)

In the example above, the library path is /lib64.

Happy hacking.
