# Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

{
  'variables': {
    'appname': 'Brackets',
    'chromium_code': 1,
    'conditions': [
      [ 'OS=="mac"', {
        # Don't use clang with CEF binary releases due to Chromium tree structure dependency.
        'clang': 0,
      }]
    ]
  },
  'includes': [
    # Bring in the source file lists for appshell.
    'appshell_paths.gypi',
  ],
  'target_defaults':
  {
    'xcode_settings':
      {
        'SDKROOT': '',
        'CLANG_CXX_LANGUAGE_STANDARD' : 'c++0x',
        'COMBINE_HIDPI_IMAGES': 'YES',
      },
  },
  'targets': [
    {
      'target_name': '<(appname)',
      'type': 'executable',
      'mac_bundle': 1,
      'msvs_guid': '6617FED9-C5D4-4907-BF55-A90062A6683F',
      'dependencies': [
        'libcef_dll_wrapper',
      ],
      'defines': [
        'USING_CEF_SHARED',
      ],
      'include_dirs': [
        '.',
        '/usr/include/gtk-2.0/',
        '/usr/include/glib-2.0/',
        '/usr/include/gdk-pixbuf-2.0/',
        '/usr/include/atk-1.0/',
        '/usr/include/cairo/',
        '/usr/include/pango-1.0/',
        '/usr/lib/glib-2.0/include/',
        '/usr/lib/gtk-2.0/include/',
      ],
      'sources': [
        '<@(includes_common)',
        '<@(includes_wrapper)',
        '<@(appshell_sources_common)',
      ],
      'mac_bundle_resources': [
        '<@(appshell_bundle_resources_mac)',
      ],
      'mac_bundle_resources!': [
        # TODO(mark): Come up with a fancier way to do this (mac_info_plist?)
        # that automatically sets the correct INFOPLIST_FILE setting and adds
        # the file to a source group.
        'appshell/mac/Info.plist',
      ],
      'xcode_settings': {
        'INFOPLIST_FILE': 'appshell/mac/Info.plist',
        # Necessary to avoid an "install_name_tool: changing install names or
        # rpaths can't be redone" error.
        'OTHER_LDFLAGS': ['-Wl,-headerpad_max_install_names'],
        # Target build path.
        'SYMROOT': 'xcodebuild',
        'GCC_TREAT_WARNINGS_AS_ERRORS': 'NO',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
      },
      'conditions': [
        ['OS=="win"', {
          'configurations': {
            'Common_Base': {
              'msvs_configuration_attributes': {
                'OutputDirectory': '$(ConfigurationName)',
              },
            },
          },
          'msvs_settings': {
            'VCLinkerTool': {
              # Set /SUBSYSTEM:WINDOWS.
              'SubSystem': '2',
              'EntryPointSymbol' : 'wWinMainCRTStartup',
            },
          },
          'link_settings': {
            'libraries': [
              '-lcomctl32.lib',
              '-lshlwapi.lib',
              '-lrpcrt4.lib',
              '-lopengl32.lib',
              '-lglu32.lib',
              '-llib/$(ConfigurationName)/libcef.lib'
            ],
          },
          'sources': [
            '<@(includes_win)',
            '<@(appshell_sources_win)',
          ],
        }],
        [ 'OS=="mac"', {
          'product_name': '<(appname)',
          'dependencies': [
            'appshell_helper_app',
          ],
          'copies': [
            {
              # Add library dependencies to the bundle.
              'destination': '<(PRODUCT_DIR)/<(appname).app/Contents/Frameworks/Chromium Embedded Framework.framework/Libraries/',
              'files': [
                '$(CONFIGURATION)/libcef.dylib',
              ],
            },
            {
              # Add other resources to the bundle.
              'destination': '<(PRODUCT_DIR)/<(appname).app/Contents/Frameworks/Chromium Embedded Framework.framework/',
              'files': [
                'Resources/',
              ],
            },
            {
              # Add the helper app.
              'destination': '<(PRODUCT_DIR)/<(appname).app/Contents/Frameworks',
              'files': [
                '<(PRODUCT_DIR)/<(appname) Helper.app',
              ],
            },
          ],
          'postbuilds': [
            {
              'postbuild_name': 'Fix Framework Link',
              'action': [
                'install_name_tool',
                '-change',
                '@executable_path/libcef.dylib',
                '@executable_path/../Frameworks/Chromium Embedded Framework.framework/Libraries/libcef.dylib',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
            {
              # This postbuid step is responsible for creating the following
              # helpers:
              #
              # <(appname) Helper EH.app and <(appname) Helper NP.app are created
              # from <(appname) Helper.app.
              #
              # The EH helper is marked for an executable heap. The NP helper
              # is marked for no PIE (ASLR).
              'postbuild_name': 'Make More Helpers',
              'action': [
                'tools/make_more_helpers.sh',
                'Frameworks',
                '<(appname)',
              ],
            },
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(CONFIGURATION)/libcef.dylib',
            ],
          },
          'sources': [
            '<@(includes_mac)',
            '<@(appshell_sources_mac)',
          ],
        }],
        [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'include_dirs': [
            '.',
            '/usr/include/gtk-2.0/',
            '/usr/include/glib-2.0/',
            '/usr/include/gdk-pixbuf-2.0/',
            '/usr/include/atk-1.0/',
            '/usr/include/cairo/',
            '/usr/include/pango-1.0/',
            '/usr/lib/glib-2.0/include/',
            '/usr/lib/gtk-2.0/include/',
          ],
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/files',
              'files': [
                '<@(appshell_bundle_resources_linux)',
              ],
            },
          ],
          'sources': [
            '<@(includes_linux)',
            '<@(appshell_sources_linux)',
          ],
        }],
      ],
    },
    {
      'target_name': 'libcef_dll_wrapper',
      'type': 'static_library',
      'msvs_guid': 'A9D6DC71-C0DC-4549-AEA0-3B15B44E86A9',
      'defines': [
        'USING_CEF_SHARED',
      ],
      'configurations': {
        'Common_Base': {
          'msvs_configuration_attributes': {
            'OutputDirectory': '$(ConfigurationName)',
          },
        },
      },
      'include_dirs': [
        '.',
        '/usr/include/gtk-2.0/',
        '/usr/include/glib-2.0/',
        '/usr/include/gdk-pixbuf-2.0/',
        '/usr/include/atk-1.0/',
        '/usr/include/cairo/',
        '/usr/include/pango-1.0/',
        '/usr/lib/glib-2.0/include/',
        '/usr/lib/gtk-2.0/include/',
      ],
      'sources': [
        '<@(includes_common)',
        '<@(includes_capi)',
        '<@(includes_wrapper)',
        '<@(libcef_dll_wrapper_sources_common)',
      ],
      'xcode_settings': {
        # Target build path.
        'SYMROOT': 'xcodebuild',
        'GCC_TREAT_WARNINGS_AS_ERRORS': 'NO',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
      },
    },
  ],
  'conditions': [
    ['OS=="mac"', {
      'targets': [
        {
          'target_name': 'appshell_helper_app',
          'type': 'executable',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'product_name': '<(appname) Helper',
          'mac_bundle': 1,
          'dependencies': [
            'libcef_dll_wrapper',
          ],
          'defines': [
            'USING_CEF_SHARED',
          ],
          'include_dirs': [
            '.',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(CONFIGURATION)/libcef.dylib',
            ],
          },
          'sources': [
            '<@(appshell_sources_mac_helper)',
          ],
          # TODO(mark): Come up with a fancier way to do this.  It should only
          # be necessary to list helper-Info.plist once, not the three times it
          # is listed here.
          'mac_bundle_resources!': [
            'appshell/mac/helper-Info.plist',
          ],
          # TODO(mark): For now, don't put any resources into this app.  Its
          # resources directory will be a symbolic link to the browser app's
          # resources directory.
          'mac_bundle_resources/': [
            ['exclude', '.*'],
          ],
          'xcode_settings': {
            'INFOPLIST_FILE': 'appshell/mac/helper-Info.plist',
            # Necessary to avoid an "install_name_tool: changing install names or
            # rpaths can't be redone" error.
            'OTHER_LDFLAGS': ['-Wl,-headerpad_max_install_names'],
            'SYMROOT': 'xcodebuild',
            'GCC_TREAT_WARNINGS_AS_ERRORS': 'NO',
            'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
          },
          'postbuilds': [
            {
              # The framework defines its load-time path
              # (DYLIB_INSTALL_NAME_BASE) relative to the main executable
              # (chrome).  A different relative path needs to be used in
              # appshell_helper_app.
              'postbuild_name': 'Fix Framework Link',
              'action': [
                'install_name_tool',
                '-change',
                '@executable_path/libcef.dylib',
                '@executable_path/../../../../Frameworks/Chromium Embedded Framework.framework/Libraries/libcef.dylib',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
          ],
        },  # target appshell_helper_app
      ],
    }],  # OS=="mac"
  ],
}


# Custom linker command:
# g++   -o out/Default/Brackets -Wl,--start-group out/Default/obj.target/Brackets/appshell/appshell_extensions.o out/Default/obj.target/Brackets/appshell/appshell_extensions_gtk.o out/Default/obj.target/Brackets/appshell/cefclient.o out/Default/obj.target/Brackets/appshell/client_app.o out/Default/obj.target/Brackets/appshell/client_app_delegates.o out/Default/obj.target/Brackets/appshell/client_handler.o out/Default/obj.target/Brackets/appshell/client_switches.o out/Default/obj.target/Brackets/appshell/string_util.o out/Default/obj.target/Brackets/appshell/cefclient_gtk.o out/Default/obj.target/Brackets/appshell/client_handler_gtk.o out/Default/obj.target/Brackets/appshell/client_app_gtk.o out/Default/obj.target/Brackets/appshell/resource_util_linux.o out/Default/obj.target/libcef_dll_wrapper.a deps/cef/Release/lib.target/libcef.so -lgtk-x11-2.0  -lgobject-2.0 -lpthread.so -lglib-2.0 -Wl,--end-group
