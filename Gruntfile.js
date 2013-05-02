/*
 * Copyright (c) 2013 Adobe Systems Incorporated. All rights reserved.
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
/*jslint regexp:true*/
/*global module, require, process*/
module.exports = function (grunt) {
    "use strict";
    
    var common  = require("./tasks/common")(grunt),
        resolve = common.resolve,
        staging = (common.platform() === "mac") ? "installer/mac/staging/<%= build.name %>.app/Contents" : "installer/win/staging";

    grunt.initConfig({
        "pkg":              grunt.file.readJSON("package.json"),
        "curl-dir": {
            /* linux not supported yet */
            /*
            linux: {
                dest        : "<%= cef_zip %>",
                src         : "https://docs.google.com/file/d/0B7as0diokeHxeTNqZFIyNWZKSWM/edit?usp=sharing"
            },
            */
            /* mac */
            "cef-mac": {
                "dest"      : "downloads/",
                "src"       : "http://chromiumembedded.googlecode.com/files/cef_binary_<%= cef.version %>_macosx.zip"
            },
            "node-mac": {
                "dest"      : "downloads/",
                "src"       : "http://nodejs.org/dist/v<%= node.version %>/node-v<%= node.version %>-darwin-x86.tar.gz"
            },
            /* win */
            "cef-win": {
                "dest"      : "downloads/",
                "src"       : "http://chromiumembedded.googlecode.com/files/cef_binary_<%= cef.version %>_windows.zip"
            },
            "node-win": {
                "dest"      : "downloads/",
                "src"       : ["http://nodejs.org/dist/v<%= node.version %>/node.exe",
                               "http://nodejs.org/dist/npm/npm-<%= npm.version %>.zip"]
            }
        },
        "clean": {
            "downloads"         : ["downloads"],
            "installer-mac"     : ["installer/mac/*.dmg"],
            "installer-win"     : ["installer/win/*.msi"],
            "staging-mac"       : ["installer/mac/staging"],
            "staging-win"       : ["installer/win/staging"],
            "www"               : ["<%= build.staging %>/www", "<%= build.staging %>/samples"]
        },
        "copy": {
            "win": {
                "files": [
                    {
                        "expand"    : true,
                        "cwd"       : "Release/",
                        "src"       : [
                            "**",
                            "!*.pdb",
                            "!dev/**",
                            "!obj/**",
                            "!lib/**",
                            "!avformat-54.dll",
                            "!avutil-51.dll",
                            "!avcodec-54.dll",
                            "!d3dcompiler_43.dll",
                            "!d3dx9_43.dll",
                            "!libEGL.dll",
                            "!libGLESv2.dll",
                            "!cefclient.exe",
                            "!debug.log"
                        ],
                        "dest"      : "installer/win/staging/"
                    }
                ]
            },
            // FIXME: see stage-mac task issues with copying .app bundles
            /*
            "mac": {
                "files": [
                    {
                        "expand"    : true,
                        "cwd"       : "xcodebuild/Release/<%= build.name %>.app/",
                        "src"       : ["**"],
                        "dest"      : "installer/mac/staging/<%= build.name %>.app/"
                    }
                ]
            },
            */
            "www": {
                "files": [
                    {
                        "dot"       : true,
                        "expand"    : true,
                        "cwd"       : "<%= git.www.repo %>/src",
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
            }
        },
        "jshint": {
            "all"           : ["Gruntfile.js", "tasks/**/*.js"],
            "options": {
                "jshintrc"  : ".jshintrc"
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
            "version"       : "3.1180.823"
        },
        "node": {
            "version"       : "0.8.20"
        },
        "npm": {
            "version"       : "1.2.11"
        }
    });

    grunt.loadTasks("tasks");
    grunt.loadNpmTasks("grunt-contrib-jshint");
    grunt.loadNpmTasks("grunt-contrib-copy");
    grunt.loadNpmTasks("grunt-contrib-clean");
    grunt.loadNpmTasks("grunt-curl");

    grunt.registerTask("default", ["setup", "build"]);
};
