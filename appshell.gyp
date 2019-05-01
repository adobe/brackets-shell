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
        'ARCHS': "$(ARCHS_STANDARD_64_BIT)",
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
        'deps/icu/include',
      ],
      'sources': [
        '<@(includes_common)',
        '<@(includes_wrapper)',
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
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
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
              '-luser32.lib',
              '-lcomdlg32.lib',
              '-lshell32.lib',
              '-lole32.lib',
              '-lgdi32.lib',
              '-ldeps/icu/lib/icuin.lib',
              '-ldeps/icu/lib/icuuc.lib',
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
            },
            {
              # Copy ICU dlls
              'destination': '<(PRODUCT_DIR)',
              'files': ['deps/icu/bin/icuuc58.dll', 'deps/icu/bin/icuin58.dll', 'deps/icu/bin/icudt58.dll'],
            },
            {
              # Copy VS2015 CRT dlls
              'destination': '<(PRODUCT_DIR)',
              'files': [
                'deps/vs-crt/api-ms-win-core-console-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-handle-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-processenvironment-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-synch-l1-2-0.dll',
                'deps/vs-crt/api-ms-win-crt-filesystem-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-runtime-l1-1-0.dll',
                'deps/vs-crt/vccorlib140.dll',
                'deps/vs-crt/api-ms-win-core-datetime-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-heap-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-processthreads-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-sysinfo-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-heap-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-stdio-l1-1-0.dll',
                'deps/vs-crt/vcruntime140.dll',
                'deps/vs-crt/api-ms-win-core-debug-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-interlocked-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-processthreads-l1-1-1.dll',
                'deps/vs-crt/api-ms-win-core-timezone-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-locale-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-string-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-errorhandling-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-libraryloader-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-profile-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-util-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-math-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-time-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-file-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-localization-l1-2-0.dll',
                'deps/vs-crt/api-ms-win-core-rtlsupport-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-conio-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-multibyte-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-utility-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-file-l1-2-0.dll',
                'deps/vs-crt/api-ms-win-core-memory-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-string-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-convert-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-private-l1-1-0.dll',
                'deps/vs-crt/msvcp140.dll',
                'deps/vs-crt/api-ms-win-core-file-l2-1-0.dll',
                'deps/vs-crt/api-ms-win-core-namedpipe-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-core-synch-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-environment-l1-1-0.dll',
                'deps/vs-crt/api-ms-win-crt-process-l1-1-0.dll',
                'deps/vs-crt/ucrtbase.dll',
                ],
            },
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
              # Add libraries and helper app.
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
              'deps/icu/lib/libicuuc.a',
              'deps/icu/lib/libicui18n.a',
              'deps/icu/lib/libicudata.a',
            ],
          },
          'sources': [
            '<@(includes_mac)',
            '<@(appshell_sources_mac)',
          ],
        }],
        [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
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
              'destination': '<(PRODUCT_DIR)/files',
              'files': [
                '<@(appshell_bundle_resources_linux)',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)/',
              'files': [
                'Resources/cef.pak',
                'Resources/cef_100_percent.pak',
                'Resources/cef_200_percent.pak',
                'Resources/cef_extensions.pak',
                'Resources/devtools_resources.pak',
                'Resources/icudtl.dat',
                'Resources/locales/',
                '$(BUILDTYPE)/chrome-sandbox',
                '$(BUILDTYPE)/libcef.so',
                '$(BUILDTYPE)/natives_blob.bin',
                '$(BUILDTYPE)/snapshot_blob.bin',
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
          'dependencies': [
            'gtk',
          ],
          'link_settings': {
            'ldflags': [
              '-pthread',
              '-Wl,-rpath,\$$ORIGIN/',
              '<(march)'
            ],
            'libraries': [
              "$(BUILDTYPE)/libcef.so",
              "-lX11",
              'appshell_extensions_js.o',
              'deps/icu/lib/libicuuc.a',
              'deps/icu/lib/libicuio.a',
              'deps/icu/lib/libicui18n.a',
              'deps/icu/lib/libicudata.a',
              '-ldl',
            ],
          },
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
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
      },
      'conditions': [
        ['OS=="linux"', {
          'cflags': [
            '<(march)',
          ],
          'default_configuration': 'Release',
          'configurations': {
            'Release': {},
            'Debug': {},
          },
        }],
        [ 'OS=="win" and multi_threaded_dll', {
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
        }],
      ],
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
            'deps/icu/include',
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
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
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
    [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
      'targets': [
        {
          'target_name': 'gtk',
          'type': 'none',
          'variables': {
            # gtk requires gmodule, but it does not list it as a dependency
            # in some misconfigured systems.
            'gtk_packages': 'gmodule-2.0 gtk+-2.0 gthread-2.0 gtk+-unix-print-2.0',
          },
          'direct_dependent_settings': {
            'cflags': [
              '$(shell <(pkg-config) --cflags <(gtk_packages))',
            ],
          },
          'link_settings': {
            'ldflags': [
              '$(shell <(pkg-config) --libs-only-L --libs-only-other <(gtk_packages))',
            ],
            'libraries': [
              '$(shell <(pkg-config) --libs-only-l <(gtk_packages))',
            ],
          },
        },
      ],
    }],  # OS=="linux" or OS=="freebsd" or OS=="openbsd"
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