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
/*jslint es5:true*/
/*global module, require, process*/
module.exports = function (grunt) {
    "use strict";

    var fs              = require("fs"),
        zlib            = require("zlib"),
        Q               = require("q"),
        /* constants */
        CEF_LINUX_URL   = "https://docs.google.com/file/d/0B7as0diokeHxeTNqZFIyNWZKSWM/edit?usp=sharing",
        CEF_VERSION     = "3.1180.823",
        CEF_ZIP         = "cef.zip",
        CEF_MAPPING     = {
            "deps/cef/Debug": "Debug",
            "deps/cef/include": "include",
            "deps/cef/libcef_dll": "libcef_dll",
            "deps/cef/Release": "Release",
            "deps/cef/Resources": "Resources",
            "deps/cef/tools": "tools"
        },
        /* use promises instead of callbacks */
        unzip           = Q.denodeify(zlib.unzip),
        link;
    
    // cross-platform symbolic link
    link = (function () {
        var symlink;
        
        if (process.platform === "win32") {
            symlink = Q.denodeify(fs.symlink);
            
            return function (srcpath, destpath) {
                return symlink(srcpath, destpath, "junction");
            };
        } else {
            symlink = Q.denodeify(fs.link);
            
            return function (srcpath, destpath) {
                return symlink(srcpath, destpath);
            };
        }
    }());
    
    function getCefInfo() {
        var platform,
            cefZipName,
            cefUrl;
        
        if (process.platform === "linux") {
            cefUrl = CEF_LINUX_URL;
        } else if (process.platform === "win32") {
            platform = "windows";
        } else if (process.platform === "darwin") {
            platform = "macosx";
        }
        
        cefZipName = "cef_binary_" + CEF_VERSION + "_" + platform;
        
        if (!cefUrl) {
            cefUrl = "http://chromiumembedded.googlecode.com/files/" + cefZipName + ".zip";
        }
        
        return {
            cefZipName: cefZipName,
            cefUrl: cefUrl
        };
    }
    
    // task: clean-cef
    grunt.registerTask("clean-cef", "Removes CEF binaries and linked folders", function () {
        var path;
        
        Object.keys(CEF_MAPPING).forEach(function (key, index) {
            path = CEF_MAPPING[key];
            
            if (grunt.file.exists(path)) {
                grunt.file.delete(path);
            }
        });
    });
    
    // task: download-cef
    grunt.registerTask("download-cef", "Download CEF binary", function () {
        var done        = this.async(),
            info        = getCefInfo(),
            curlConfig  = {},
            unzipPromise;
        
        // check if the correct version was already downloaded
        if (grunt.file.exists("deps/cef/" + info.cefZipName + ".txt")) {
            grunt.log.write("You already have the correct version of CEF downloaded.");
        } else {
            // download zip file
            curlConfig.src = info.cefUrl;
            curlConfig.dest = CEF_ZIP;
            
            grunt.config.set("curl.long", curlConfig);
            grunt.log.write("Downloading CEF binary distribution");
            grunt.task.run("curl");
        }
        
        // delete existing directories
        grunt.task.run("clean-cef");
        
        // unzip downloaded zip file
        unzipPromise = unzip(grunt.file.read("cef.zip", { encoding: null }))
            .then(function () {
                grunt.log.write("unzipped");
            })
            .then(function () {
                done();
            }, function (error) {
                done(false);
            });
    });
};