/*
 * Copyright (c) 2013 - present Adobe Systems Incorporated. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

"use strict";

module.exports = function (grunt) {
    var common  = require("./tasks/common")(grunt),
        platform = common.platform(),
        staging,
        cef_version = "3.2623.1397";

    if (platform === "mac") {
        staging = "installer/mac/staging/<%= build.name %>.app/Contents";
    } else if (platform === "win") {
        staging = "installer/win/staging";
    } else {
        staging = "installer/linux/debian/package-root/opt/brackets";
    }

    if (platform === "linux") {
        cef_version = "3.2785.1486";
    }

    grunt.initConfig({
        "pkg":              grunt.file.readJSON("package.json"),
        "config-json":      staging + "/www/config.json",
        "downloads":        grunt.option("downloads") || "downloads",
        "curl-dir": {
            /* linux */
            "cef-linux32": {
                "dest"      : "<%= downloads %>",
                "src"       : "<%= cef.url %>/cef_binary_<%= cef.version %>_linux32_release.zip"
            },
            "cef-linux64": {
                "dest"      : "<%= downloads %>",
                "src"       : "<%= cef.url %>/cef_binary_<%= cef.version %>_linux64_release.zip"
            },
            "node-linux32": {
                "dest"      : "<%= downloads %>",
                "src"       : "http://nodejs.org/dist/v<%= node.version %>/node-v<%= node.version %>-linux-x86.tar.gz"
            },
            "node-linux64": {
                "dest"      : "<%= downloads %>",
                "src"       : "http://nodejs.org/dist/v<%= node.version %>/node-v<%= node.version %>-linux-x64.tar.gz"
            },
            "icu-linux32": {
                "dest"      : "<%= downloads %>",
                "src"       : "<%= icu.url %>/icu_<%= icu.version %>_linux32_release.zip"
            },
            "icu-linux64": {
                "dest"      : "<%= downloads %>",
                "src"       : "<%= icu.url %>/icu_<%= icu.version %>_linux64_release.zip"
            },
            /* mac */
            "cef-mac": {
                "dest"      : "<%= downloads %>",
                "src"       : "<%= cef.url %>/cef_binary_<%= cef.version %>_macosx64.zip"
            },
            "cef-mac-symbols": {
                "src"  : "<%= cef.url %>/cef_binary_<%= cef.version %>_macosx32_release_symbols.zip",
                "dest" : "<%= downloads %>/cefsymbols"
            },
            "node-mac": {
                "dest"      : "<%= downloads %>",
                "src"       : "http://nodejs.org/dist/v<%= node.version %>/node-v<%= node.version %>-darwin-x64.tar.gz"
            },
            "icu-mac": {
                "dest"      :  "<%= downloads %>",
                "src"       :  "<%= icu.url %>/icu_<%= icu.version %>_macosx64.zip"
            },
            /* win */
            "cef-win": {
                "dest"      : "<%= downloads %>",
                "src"       : "<%= cef.url %>/cef_binary_<%= cef.version %>_windows32.zip"
            },
            "cef-win-symbols": {
                "src"  : ["<%= cef.url %>/cef_binary_<%= cef.version %>_windows32_debug_symbols.zip", "<%= cef.url %>/cef_binary_<%= cef.version %>_windows32_release_symbols.zip"],
                "dest" : "<%= downloads %>/cefsymbols"
            },
            "node-win": {
                "dest"      : "<%= downloads %>",
                "src"       : "http://nodejs.org/dist/v<%= node.version %>/win-x86/node.exe"
            },
            "icu-win": {
                "dest"      :  "<%= downloads %>",
                "src"       :  "<%= icu.url %>/icu_<%= icu.version %>_windows32.zip"
            },
            "vs-crt-win": {
                "dest"      :  "<%= downloads %>",
                "src"       :  "<%= vsCrt.url %>/vs<%= vsCrt.version %>-crt-ia32.zip"
            }
        },
        "clean": {
            "downloads"         : ["downloads"],
            "installer-mac"     : ["installer/mac/*.dmg"],
            "installer-win"     : ["installer/win/*.msi"],
            "installer-linux"   : ["installer/linux/debian/*.deb"],
            "staging-mac"       : ["installer/mac/staging"],
            "staging-win"       : ["installer/win/staging"],
            "staging-linux"     : ["<%= build.staging %>"],
            "www"               : ["<%= build.staging %>/www", "<%= build.staging %>/samples"]
        },
        "copy": {
            "win": {
                "files": [
                    {
                        "expand"    : true,
                        "cwd"       : "Release/",
                        "src"       : [
                            "locales/**",
                            "node-core/**",
                            "Brackets.exe",
                            "node.exe",
                            "cef.pak",
                            "cef_100_percent.pak",
                            "cef_200_percent.pak",
                            "devtools_resources.pak",
                            "icudtl.dat",
                            "libcef.dll",
                            "icuuc58.dll",
                            "icuin58.dll",
                            "icudt58.dll",
                            "api-ms-win-core-console-l1-1-0.dll",
                            "api-ms-win-core-handle-l1-1-0.dll",
                            "api-ms-win-core-processenvironment-l1-1-0.dll",
                            "api-ms-win-core-synch-l1-2-0.dll",
                            "api-ms-win-crt-filesystem-l1-1-0.dll",
                            "api-ms-win-crt-runtime-l1-1-0.dll",
                            "vccorlib140.dll",
                            "api-ms-win-core-datetime-l1-1-0.dll",
                            "api-ms-win-core-heap-l1-1-0.dll",
                            "api-ms-win-core-processthreads-l1-1-0.dll",
                            "api-ms-win-core-sysinfo-l1-1-0.dll",
                            "api-ms-win-crt-heap-l1-1-0.dll",
                            "api-ms-win-crt-stdio-l1-1-0.dll",
                            "vcruntime140.dll",
                            "api-ms-win-core-debug-l1-1-0.dll",
                            "api-ms-win-core-interlocked-l1-1-0.dll",
                            "api-ms-win-core-processthreads-l1-1-1.dll",
                            "api-ms-win-core-timezone-l1-1-0.dll",
                            "api-ms-win-crt-locale-l1-1-0.dll",
                            "api-ms-win-crt-string-l1-1-0.dll",
                            "api-ms-win-core-errorhandling-l1-1-0.dll",
                            "api-ms-win-core-libraryloader-l1-1-0.dll",
                            "api-ms-win-core-profile-l1-1-0.dll",
                            "api-ms-win-core-util-l1-1-0.dll",
                            "api-ms-win-crt-math-l1-1-0.dll",
                            "api-ms-win-crt-time-l1-1-0.dll",
                            "api-ms-win-core-file-l1-1-0.dll",
                            "api-ms-win-core-localization-l1-2-0.dll",
                            "api-ms-win-core-rtlsupport-l1-1-0.dll",
                            "api-ms-win-crt-conio-l1-1-0.dll",
                            "api-ms-win-crt-multibyte-l1-1-0.dll",
                            "api-ms-win-crt-utility-l1-1-0.dll",
                            "api-ms-win-core-file-l1-2-0.dll",
                            "api-ms-win-core-memory-l1-1-0.dll",
                            "api-ms-win-core-string-l1-1-0.dll",
                            "api-ms-win-crt-convert-l1-1-0.dll",
                            "api-ms-win-crt-private-l1-1-0.dll",
                            "msvcp140.dll",
                            "api-ms-win-core-file-l2-1-0.dll",
                            "api-ms-win-core-namedpipe-l1-1-0.dll",
                            "api-ms-win-core-synch-l1-1-0.dll",
                            "api-ms-win-crt-environment-l1-1-0.dll",
                            "api-ms-win-crt-process-l1-1-0.dll",
                            "ucrtbase.dll",
                            "natives_blob.bin",
                            "snapshot_blob.bin",
                            "command/**"
                        ],
                        "dest"      : "installer/win/staging/"
                    }
                ]
            },
            "winInstallerDLLs": {
                "files": [
                    {
                        "flatten"   : true,
                        "expand"    : true,
                        "src"       : [
                            "installer/win/LaunchBrackets/LaunchBrackets/bin/Release/LaunchBrackets.CA.dll",
                            "installer/win/BracketsConfigurator/BracketsConfigurator/bin/Release/BracketsConfigurator.CA.dll"
                        ],
                        "dest"      : "installer/win/"
                    }
                ]
            },
            "mac": {
                "files": [
                    {
                        "expand"    : true,
                        "cwd"       : "xcodebuild/Release/<%= build.name %>.app/",
                        "src"       : [
                            "**",
                            "!**/Contents/Frameworks/Chromium Embedded Framework.framework/Libraries/**"
                        ],
                        "dest"      : "installer/mac/staging/<%= build.name %>.app/"
                    }
                ],
                options: {
                    mode: true
                }
            },
            "cefplist" : {
                "files": [
                    {
                        "src"  : "CEF-Info.plist",
                        "dest" : "installer/mac/staging/<%= build.name %>.app/Contents/Frameworks/Chromium Embedded Framework.framework/Resources/Info.plist"
                    }
                ]
            },
            "linux": {
                "files": [
                    {
                        "expand"    : true,
                        "cwd"       : "out/Release/",
                        "src"       : [
                            "lib/**",
                            "locales/**",
                            "node-core/**",
                            "appshell*.png",
                            "Brackets",
                            "Brackets-node",
                            "cef.pak",
                            "cef_100_percent.pak",
                            "cef_200_percent.pak",
                            "devtools_resources.pak",
                            "cef_extensions.pak",
                            "icudtl.dat",
                            "libcef.so",
                            "natives_blob.bin",
                            "snapshot_blob.bin",
                            "chrome-sandbox",
                        ],
                        "dest"      : "<%= build.staging %>"
                    },
                    {
                        "expand"    : true,
                        "cwd"       : "installer/linux/debian/",
                        "src"       : [
                            "brackets.desktop",
                            "brackets"
                        ],
                        "dest"      : "<%= build.staging %>"
                    }
                ]
            },
            "www": {
                "files": [
                    {
                        "dot"       : true,
                        "expand"    : true,
                        "cwd"       : "<%= git.www.repo %>/dist",
                        "src"       : ["**", "!**/.git*"],
                        "dest"      : "<%= build.staging %>/www/"
                    }
                ]
            },
            "samples": {
                "files": [
                    {
                        "dot"       : true,
                        "expand"    : true,
                        "cwd"       : "<%= git.www.repo %>/samples",
                        "src"       : ["**"],
                        "dest"      : "<%= build.staging %>/samples/"
                    }
                ]
            }
        },
        "unzip": {
            "cef": {
                "src"       : "<%= cef_zip %>",
                "dest"      : "deps/cef"
            },
            "icu": {
                "src"       : "<%= icu_zip %>",
                "dest"      : "deps/icu"
            }
        },
        "eslint": {
            "all"           : [
                "Gruntfile.js",
                "tasks/**/*.js",
                "appshell/node-core/*.js"
            ],
            "options": {
                "quiet"     : true
            }
        },
        "build": {
            "name"              : "Brackets",
            "staging"           : staging
        },
        "git": {
            "www": {
                "repo"      : "../brackets",    // TODO user configurable?
                "branch"    : grunt.option("www-branch") || ""
            },
            "shell": {
                "repo"      : ".",
                "branch"    : grunt.option("shell-branch") || ""
            }
        },
        "cef": {
            "url"           : "http://s3.amazonaws.com/files.brackets.io/cef",
            "version"       : cef_version
        },
        "node": {
            "version"       : "6.11.0"
        },
        "icu": {
            "url"           : "http://s3.amazonaws.com/files.brackets.io/icu",
            "version"       : "58"
        },
        "vsCrt": {
            "url"           : "http://s3.amazonaws.com/files.brackets.io/vs-crt",
            "version"       : "2015"
        },
    });

    grunt.loadTasks("tasks");
    grunt.loadNpmTasks("grunt-eslint");
    grunt.loadNpmTasks("grunt-contrib-copy");
    grunt.loadNpmTasks("grunt-contrib-clean");
    grunt.loadNpmTasks("grunt-curl");

    grunt.registerTask("default", ["setup", "build"]);
};