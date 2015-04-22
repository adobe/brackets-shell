# Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

{
  'variables': {
    'pkg-config': 'pkg-config',
    'target_arch%': 'environment',
    'chromium_code': 1,
    'framework_name': 'Chromium Embedded Framework',
    'linux_use_gold_binary': 0,
    'linux_use_gold_flags': 0,
    'conditions': [
      [ 'OS=="mac"', {
        # Don't use clang with CEF binary releases due to Chromium tree structure dependency.
        'clang': 0,
      }],
      [ 'OS=="win"', {
        'multi_threaded_dll%': 0,
      }],
    ]
  },
  'includes': [
    # Bring in the configuration vars
    'appshell_config.gypi',
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
        'ARCHS': "$(ARCHS_STANDARD_32_BIT)",
        'FRAMEWORK_SEARCH_PATHS': [
          '$(inherited)',
          '$(CONFIGURATION)'
        ]
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
          'variables': {
            'win_exe_compatibility_manifest': 'compatibility.manifest',
            'xparams': "/efy",
          },
          'actions': [
            {
              'action_name': 'copy_resources',
              'msvs_cygwin_shell': 0,
              'inputs': [],
              'outputs': [
                '<(PRODUCT_DIR)/copy_resources.stamp',
              ],
              'action': [
                'xcopy <(xparams)',
                'Resources\*',
                '$(OutDir)',
              ],
            },
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # Set /SUBSYSTEM:WINDOWS.
              'SubSystem': '2',
            },
            'VCManifestTool': {
              'AdditionalManifestFiles': [
                'appshell.exe.manifest',
              ],
            },
          },
          'link_settings': {
            'libraries': [
              '-lcomctl32.lib',
              '-lshlwapi.lib',
              '-lrpcrt4.lib',
              '-lopengl32.lib',
              '-lglu32.lib',
              '-l$(ConfigurationName)/libcef.lib',
            ],
          },
          'library_dirs': [
            # Needed to find cef_sandbox.lib using #pragma comment(lib, ...).
            '$(ConfigurationName)',
          ],
          'sources': [
            '<@(includes_win)',
            '<@(appshell_sources_win)',
          ],
          'copies': [
            {
              # Copy node executable to the output directory
              'destination': '<(PRODUCT_DIR)',
              'files': ['deps/node/node.exe'],
            },
            {
              # Copy node server files to the output directory
              # The '/' at the end of the 'files' directory is very important and magically
              # causes 'xcopy' to get used instead of 'copy' for recursive copies.
              # This seems to be an undocumented feature of gyp.
             'destination': '<(PRODUCT_DIR)',
              'files': ['appshell/node-core/'],
            },
            {
              # Copy resources
              'destination': '<(PRODUCT_DIR)',
              'files': ['Resources/cef.pak', 'Resources/devtools_resources.pak'],
            },
            {
              # Copy locales
              'destination': '<(PRODUCT_DIR)',
              'files': ['Resources/locales/'],
            },
            {
              # Copy windows command line script
              'destination': '<(PRODUCT_DIR)command',
              'files': ['scripts/brackets.bat', 'scripts/brackets'],
            }
          ],
        }],
        [ 'OS=="win" and multi_threaded_dll', {
          'configurations': {
            'Debug': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 3,
                  'WarnAsError': 'false',
                },
              },
            },
            'Release': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 2,
                  'WarnAsError': 'false',
                },
              },
            }
          }
        }],
        [ 'OS=="mac"', {
          'product_name': '<(appname)',
          'dependencies': [
            'appshell_helper_app',
          ],
          'copies': [
            {
              # Add library dependencies to the bundle.
              'destination': '<(PRODUCT_DIR)/<(appname).app/Contents/Frameworks',
              'files': [
                '<(PRODUCT_DIR)/<(appname) Helper.app',
              ],
            },
          ],
          'postbuilds': [
            {
             'postbuild_name': 'Add framework',
              'action': [
                'cp',
                '-Rf',
                '${CONFIGURATION}/<(framework_name).framework',
                '${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Frameworks/'
              ],
            },
            {
              'postbuild_name': 'Fix Framework Link',
              'action': [
                'install_name_tool',
                '-change',
                '@executable_path/<(framework_name)',
                '@executable_path/../Frameworks/<(framework_name).framework/<(framework_name)',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
            {
              # Copy the entire "node-core" directory into the same location as the "www"
              # directory will end up. Note that the ".." in the path is necessary because
              # the EXECUTABLE_FOLDER_PATH macro resolves to multiple levels of folders.
              'postbuild_name': 'Copy node core resources',
              'action': [
                'rsync',
                '-a',
                './appshell/node-core/',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/../node-core/',
              ],
            },
            {
              'postbuild_name': 'Copy node executable',
              'action': [
                'cp',
                './deps/node/bin/Brackets-node',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/Brackets-node',
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
              '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
              '$(SDKROOT)/System/Library/Frameworks/ScriptingBridge.framework',
              '$(SDKROOT)/System/Library/Frameworks/Security.framework',
              '$(CONFIGURATION)/<(framework_name).framework/<(framework_name)',
            ],
          },
          'sources': [
            '<@(includes_mac)',
            '<@(appshell_sources_mac)',
          ],
        }],
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'actions': [
            {
              'action_name': 'appshell_extensions_js',
              'inputs': [
                'appshell/appshell_extensions.js',
              ],
              'outputs': [
                'appshell_extensions_js.o',
              ],
              'action': [
                'objcopy',
                '--input',
                'binary',
                '--output',
                '<(output_bfd)',
                '--binary-architecture',
                'i386',
                '<@(_inputs)',
                '<@(_outputs)',
              ],
              'message': 'compiling js resource'
            }
          ],
          'cflags': [
            '<!@(<(pkg-config) --cflags gtk+-2.0 gthread-2.0)',
            '<(march)',
          ],
          'include_dirs': [
            '.',
          ],
          'default_configuration': 'Release',
          'configurations': {
            'Release': {},
            'Debug': {},
          },
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/lib',
              'files': [
                '<@(appshell_bundle_libraries_linux)',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)',
              'files': [
                '<@(appshell_bundle_resources_linux)'
              ],
            },
            {
              # Copy node executable to the output directory
              'destination': '<(PRODUCT_DIR)',
              'files': ['deps/node/bin/Brackets-node'],
            },
            {
              # Copy node server files to the output directory
              # The '/' at the end of the 'files' directory is very important and magically
              # causes 'xcopy' to get used instead of 'copy' for recursive copies.
              # This seems to be an undocumented feature of gyp.
             'destination': '<(PRODUCT_DIR)',
              'files': ['appshell/node-core/'],
            },
          ],
          'sources': [
            '<@(includes_linux)',
            '<@(appshell_sources_linux)',
          ],
          'link_settings': {
            'ldflags': [
              '<!@(<(pkg-config) --libs-only-other gtk+-2.0 gthread-2.0)',
              '-Wl,-rpath,\$$ORIGIN/lib',
              '<(march)'
            ],
            'libraries': [
              '<!@(<(pkg-config) --libs-only-l gtk+-2.0 gthread-2.0)',
              '$(BUILDTYPE)/libcef.so',
              'appshell_extensions_js.o',
            ],
          },
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
      'conditions': [
        ['OS=="linux"', {
          'cflags': [
          '<!@(<(pkg-config) --cflags gtk+-2.0 gthread-2.0)',
          '<(march)',
          ],
          'default_configuration': 'Release',
          'configurations': {
            'Release': {},
            'Debug': {},
          },
        }],
        ['OS=="win" and multi_threaded_dll', {
          'configurations': {
            'Common_Base': {
              'msvs_configuration_attributes': {
                'OutputDirectory': '$(ConfigurationName)',
              },
            },
            'Debug': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 3,
                  'WarnAsError': 'false',
                },
              },
            },
            'Release': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 2,
                  'WarnAsError': 'false',
                },
              },
            }
          }
        }]
      ]
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
              '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
              '$(SDKROOT)/System/Library/Frameworks/ScriptingBridge.framework',
              '$(SDKROOT)/System/Library/Frameworks/Security.framework',
              '$(CONFIGURATION)/<(framework_name).framework/<(framework_name)',
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
            'FRAMEWORK_SEARCH_PATHS': [
              '$(inherited)',
              '$(CONFIGURATION)'
            ]
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
                '@executable_path/<(framework_name)',
                '@executable_path/../../../../Frameworks/<(framework_name).framework/<(framework_name)',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
          ],
        },  # target appshell_helper_app
      ],
    }],  # OS=="mac"
    ['target_arch=="ia32"', {
      'variables': {
        'output_bfd': 'elf32-i386',
        'march': '-m32',
      },
    }],
    ['target_arch=="x64"', {
      'variables': {
        'output_bfd': 'elf64-x86-64',
        'march': '-m64',
      },
    }],
    ['target_arch=="environment"', {
      'variables': {
        'output_bfd': '<!(uname -m | sed "s/x86_64/elf64-x86-64/;s/i.86/elf32-i386/")',
        'march': ' ',
      },
    }],
  ],
}
