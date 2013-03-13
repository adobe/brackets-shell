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

    grunt.initConfig({
        "curl-dir": {
            /* linux not supported yet */
            /*
            linux: {
                dest        : "<%= cef_zip %>",
                src         : "https://docs.google.com/file/d/0B7as0diokeHxeTNqZFIyNWZKSWM/edit?usp=sharing"
            },
            */
            /* mac */
            cef_darwin: {
                dest        : ".",
                src         : "http://chromiumembedded.googlecode.com/files/cef_binary_<%= cef_version %>_macosx.zip"
            },
            node_darwin: {
                dest        : ".",
                src         : "http://nodejs.org/dist/v<%= node_version %>/node-v<%= node_version %>-darwin-x86.tar.gz"
            },
            /* win */
            cef_win32: {
                dest        : ".",
                src         : "http://chromiumembedded.googlecode.com/files/cef_binary_<%= cef_version %>_windows.zip"
            },
            node_win32: {
                dest        : ".",
                src         : ["http://nodejs.org/dist/v<%= node_version %>/node.exe",
                               "http://nodejs.org/dist/npm/npm-<%= npm_version %>.zip"]
            }
        },
        unzip: {
            cef: {
                src: "<%= cef_zip %>",
                dest: "deps/cef"
            }
        },
        jshint: {
            all             : ["Gruntfile.js", "tasks/**/*.js"],
            /* use strict options to mimic JSLINT until we migrate to JSHINT in Brackets */
            options: {
                jshintrc    : ".jshintrc"
            }
        },
        cef_zip             : "cef.zip",
        cef_version         : "3.1180.823",
        node_version        : "0.8.20",
        npm_version         : "1.2.11"
    });

    grunt.loadTasks("tasks");
    grunt.loadNpmTasks("grunt-contrib-jshint");
    grunt.loadNpmTasks("grunt-curl");

    grunt.registerTask("default", "jshint");
};
