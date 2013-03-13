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
        child_process   = require("child_process"),
        Q               = require("q"),
        /* constants */
        CEF_MAPPING     = {
            "deps/cef/Debug": "Debug",
            "deps/cef/include": "include",
            "deps/cef/libcef_dll": "libcef_dll",
            "deps/cef/Release": "Release",
            "deps/cef/Resources": "Resources",
            "deps/cef/tools": "tools"
        },
        /* use promises instead of callbacks */
        link,
        exec            = Q.denodeify(child_process.exec),
        rename          = Q.denodeify(fs.rename);
    
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
    
    function unzip(src, dest) {
        grunt.verbose.writeln("Extracting " + src);
        return exec("unzip -q " + src + " -d " + dest);
    }
    
    // task: cef
    grunt.registerTask("cef", "Download and setup CEF", function () {
        var config  = "cef_" + process.platform,
            zipSrc  = grunt.config("curl-dir." + config + ".src"),
            zipName,
            txtName;
        
        // extract zip file name and set config property
        zipName = zipSrc.substr(zipSrc.lastIndexOf("/") + 1);
        grunt.config("cefZipName", zipName);
        grunt.config("cefConfig", config);
        
        // remove .zip extension
        txtName = zipName.substr(0, zipName.lastIndexOf(".")) + ".txt";
        
        // optionally download if CEF is not found
        if (!grunt.file.exists("deps/cef/" + txtName)) {
            grunt.task.run("cef-download");
        } else {
            grunt.verbose.writeln("Skipping CEF download. Found deps/cef/" + txtName);
        }
        
        grunt.task.run(["cef-clean", "cef-extract", "cef-symlinks"]);
    });
    
    // task: cef-clean
    grunt.registerTask("cef-clean", "Removes CEF binaries and linked folders", function () {
        var path;
        
        Object.keys(CEF_MAPPING).forEach(function (key, index) {
            path = CEF_MAPPING[key];
            
            if (grunt.file.exists(path)) {
                grunt.file.delete(path);
            }
        });
        
        grunt.verbose.writeln("Deleting deps/cef");
        
        if (grunt.file.exists("deps/cef")) {
            grunt.file.delete("deps/cef");
        }
    });
    
    // task: cef-download
    grunt.registerTask("cef-download", "", function () {
        // requires download-cef to set "cefZipName" in config
        grunt.task.requires(["cef"]);
        
        // run curl
        grunt.log.writeln("Downloading " + grunt.config("cefZipName"));
        grunt.task.run("curl-dir:" + grunt.config("cefConfig"));
    });
    
    // task: cef-extract
    grunt.registerTask("cef-extract", "Extract CEF zip", function () {
        // requires cef to set "cefZipName" in config
        grunt.task.requires(["cef"]);
        
        var done    = this.async(),
            zipName = grunt.config("cefZipName"),
            unzipPromise;
        
        unzipPromise = unzip(zipName, "deps");
        
        // remove .zip ext
        zipName = zipName.substr(0, zipName.lastIndexOf("."));
        
        unzipPromise.then(function () {
            var deferred = Q.defer();
            
            rename("deps/" + zipName, "deps/cef").then(function () {
                // write empty file with zip file 
                grunt.file.write("deps/cef/" + zipName + ".txt", "");
                deferred.resolve();
            });
            
            return deferred.promise;
        }).then(function () {
            done();
        }, function (err) {
            grunt.log.writeln(err);
            done(false);
        });
    });
    
    // task: cef-symlinks
    grunt.registerTask("cef-symlinks", "Create symlinks for CEF", function () {
        var done    = this.async(),
            path,
            links   = [];
        
        Object.keys(CEF_MAPPING).forEach(function (key, index) {
            path = CEF_MAPPING[key];
            links.push(link(key, path));
        });
        
        Q.all(links).then(function () {
            done();
        }, function (err) {
            done(false);
        });
    });
    
    // task: node-download
    grunt.registerTask("node", "Download Node.js binaries and setup dependencies", function () {
        var config      = "node_" + process.platform,
            nodeSrc     = grunt.config("curl-dir." + config + ".src"),
            curlTask    = "curl-dir:node_" + process.platform,
            setupTask   = "node-" + process.platform;
        
        // extract file name and set config property
        nodeSrc = Array.isArray(nodeSrc) ? nodeSrc : [nodeSrc];
        
        nodeSrc.forEach(function (value, index) {
            nodeSrc[index] = value.substr(value.lastIndexOf("/") + 1);
        });
        grunt.config("nodeSrc", nodeSrc);
        
        // queue node-clean, curl-dir:<platform>, and node-<platform>
        grunt.task.run(["node-clean", curlTask, setupTask]);
    });
    
    function nodeWriteVersion() {
        // write empty file with node_version name
        grunt.file.write("deps/node/version-" + grunt.config("node_version") + ".txt", "");
    }
    
    // task: node-win32
    grunt.registerTask("node-win32", "Download Node.js for Windows", function () {
        // requires node to set "nodeSrc" in config
        grunt.task.requires(["node"]);
        
        var done    = this.async(),
            nodeSrc = grunt.config("nodeSrc"),
            exeFile = nodeSrc[0],
            npmFile = nodeSrc[1],
            unzipPromise;
        
        grunt.file.mkdir("deps/node");
        
        // rename to Brackets-node
        rename("node.exe", "deps/node/Brackets-node.exe").then(function () {
            // unzip NPM
            return unzipPromise(npmFile, "deps/node");
        }).then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: node-darwin
    grunt.registerTask("node-darwin", "Download Node.js for Mac OSX and extract", function () {
        // requires node to set "nodeSrc" in config
        grunt.task.requires(["node"]);
        
        var done    = this.async(),
            tarFile = grunt.config("nodeSrc")[0],
            tarExec,
            tarName;
        
        grunt.verbose.writeln("Extracting " + tarFile);
        tarExec = exec("tar -xzf " + tarFile);
        
        // remove .tar.gz extension
        tarName = tarFile.substr(0, tarFile.lastIndexOf(".tar.gz"));
        
        tarExec.then(function () {
            return rename(tarName, "deps/node");
        }).then(function () {
            // Create a copy of the "node" binary as "Brackets-node". We need one named "node"
            // for npm to function properly, but we want to call the executable "Brackets-node"
            // in the final binary. Due to gyp's limited nature, we can't (easily) do this rename
            // as part of the build process.
            return rename("deps/node/bin/node", "deps/node/bin/Brackets-node");
        }).then(function () {
            nodeWriteVersion();
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: node-clean
    grunt.registerTask("node-clean", "Removes Node.js binaries", function () {
        if (grunt.file.exists("deps/node")) {
            grunt.file.delete("deps/node");
        }
    });
};