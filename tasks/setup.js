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
/*jslint vars:true*/
/*global module, require, process*/
module.exports = function (grunt) {
    "use strict";

    var common          = require("./common")(grunt),
        fs              = require("fs"),
        child_process   = require("child_process"),
        q               = require("q"),
        /* win only (lib), mac only (Resources, tools) */
        CEF_MAPPING     = {
            "deps/cef/Debug"        : "Debug",
            "deps/cef/include"      : "include",
            "deps/cef/lib"          : "lib",
            "deps/cef/libcef_dll"   : "libcef_dll",
            "deps/cef/Release"      : "Release",
            "deps/cef/Resources"    : "Resources",
            "deps/cef/tools"        : "tools"
        },
        /* use promises instead of callbacks */
        link,
        rename          = common.rename,
        exec            = common.exec;
    
    // cross-platform symbolic link
    link = (function () {
        var typeArg,
            symlink;
        
        if (process.platform === "win32") {
            typeArg = "junction";
        }
        
        symlink = q.denodeify(fs.symlink);
        
        return function (srcpath, destpath) {
            return symlink(srcpath, destpath, typeArg);
        };
    }());
    
    function unzip(src, dest) {
        grunt.verbose.writeln("Extracting " + src);
        return exec("unzip -q " + src + " -d " + dest);
    }

    // task: cef
    grunt.registerTask("cef", "Download and setup CEF", function () {
        var config      = "cef-" + common.platform() + common.arch(),
            zipSrc      = grunt.config("curl-dir." + config + ".src"),
            zipName     = zipSrc.substr(zipSrc.lastIndexOf("/") + 1),
            zipDest     = grunt.config("curl-dir." + config + ".dest") + zipName,
            txtName;
        
        // extract zip file name and set config property
        grunt.config("cefZipDest", zipDest);
        grunt.config("cefZipSrc", zipSrc);
        grunt.config("cefZipName", zipName);
        grunt.config("cefConfig", config);
        
        // remove .zip extension
        txtName = zipName.substr(0, zipName.lastIndexOf(".")) + ".txt";
        
        // optionally download if CEF is not found
        if (!grunt.file.exists("deps/cef/" + txtName)) {
            var cefTasks = ["cef-clean", "cef-extract", "cef-symlinks"];
            
            if (grunt.file.exists(zipDest)) {
                grunt.verbose.writeln("Found CEF download " + zipDest);
            } else {
                cefTasks.unshift("cef-download");
            }
            
            grunt.task.run(cefTasks);
        } else {
            grunt.verbose.writeln("Skipping CEF download. Found deps/cef/" + txtName);
        }
    });
    
    // task: cef-clean
    grunt.registerTask("cef-clean", "Removes CEF binaries and linked folders", function () {
        var path;
        
        // delete dev symlinks from "setup_for_hacking"
        common.deleteFile("Release/dev", { force: true });
        common.deleteFile("Debug/dev", { force: true });
        
        // delete symlinks to cef
        Object.keys(CEF_MAPPING).forEach(function (key, index) {
            common.deleteFile(CEF_MAPPING[key]);
        });
        
        // finally delete CEF binary\
        common.deleteFile("deps/cef", { force: true });
    });
    
    // task: cef-download
    grunt.registerTask("cef-download", "Download CEF, see curl-dir config in Gruntfile.js", function () {
        // requires download-cef to set "cefZipName" in config
        grunt.task.requires(["cef"]);
        
        // run curl
        grunt.log.writeln("Downloading " + grunt.config("cefZipSrc") + ". This may take a while...");
        grunt.task.run("curl-dir:" + grunt.config("cefConfig"));
    });
    
    // task: cef-extract
    grunt.registerTask("cef-extract", "Extract CEF zip", function () {
        // requires cef to set "cefZipName" in config
        grunt.task.requires(["cef"]);
        
        var done    = this.async(),
            zipDest = grunt.config("cefZipDest"),
            zipName = grunt.config("cefZipName"),
            unzipPromise;
        
        // unzip to deps/
        unzipPromise = unzip(zipDest, "deps");
        
        // remove .zip ext
        zipName = zipName.substr(0, zipName.lastIndexOf("."));
        
        unzipPromise.then(function () {
            // rename version stamped name to cef
            return rename("deps/" + zipName, "deps/cef");
        }).then(function () {
            if (common.platform() === "mac") {
                // FIXME figure out how to use fs.chmod to only do additive mode u+x
                return exec("chmod u+x deps/cef/tools/*");
            }
            
            // write empty file with zip file 
            grunt.file.write("deps/cef/" + zipName + ".txt", "");
            
            // return a resolved promise
            return q.resolve();
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
        
        // create symlinks
        Object.keys(CEF_MAPPING).forEach(function (key, index) {
            path = CEF_MAPPING[key];
            
            // some paths to deps/cef/* are platform specific and may not exist
            if (grunt.file.exists(key)) {
                links.push(link(key, path));
            }
        });
        
        // wait for all symlinks to complete
        q.all(links).then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: node-download
    grunt.registerTask("node", "Download Node.js binaries and setup dependencies", function () {
        var platform    = common.platform(),
            config      = "node-" + platform + common.arch(),
            nodeSrc     = grunt.config("curl-dir." + config + ".src"),
            nodeDest    = [],
            dest        = grunt.config("curl-dir." + config + ".dest"),
            curlTask    = "curl-dir:" + config,
            setupTask   = "node-" + platform,
            nodeVersion = grunt.config("node.version"),
            npmVersion  = grunt.config("npm.version"),
            txtName     = "version-" + nodeVersion + ".txt",
            missingDest = false;
        
        // extract file name and set config property
        nodeSrc = Array.isArray(nodeSrc) ? nodeSrc : [nodeSrc];
        
        // curl-dir:node-win32 defines multiple downloads, account for this array
        nodeSrc.forEach(function (value, index) {
            nodeSrc[index] = value.substr(value.lastIndexOf("/") + 1);
            nodeDest[index] = dest + nodeSrc[index];

            missingDest = missingDest || !grunt.file.exists(nodeDest[index]);
        });
        grunt.config("nodeSrc", nodeSrc);
        grunt.config("nodeDest", nodeDest);
        
        // optionally download if node is not found
        if (!grunt.file.exists("deps/node/" + txtName)) {
            if (!missingDest) {
                grunt.verbose.writeln("Found node download(s) " + nodeDest);
                grunt.task.run(["node-clean", setupTask]);
            } else {
                grunt.log.writeln("Downloading " + nodeSrc.toString() + ". This may take a while...");
                grunt.task.run([curlTask, "node-clean", setupTask]);
            }
        } else {
            grunt.verbose.writeln("Skipping Node.js download. Found deps/node/" + txtName);
        }
    });
    
    function nodeWriteVersion() {
        // write empty file with node-version name
        grunt.file.write("deps/node/version-" + grunt.config("node.version") + ".txt", "");
    }
    
    // task: node-win32
    grunt.registerTask("node-win", "Setup Node.js for Windows", function () {
        // requires node to set "nodeSrc" in config
        grunt.task.requires(["node"]);
        
        var done        = this.async(),
            nodeDest    = grunt.config("nodeDest"),
            exeFile     = nodeDest[0],
            npmFile     = nodeDest[1];
        
        grunt.file.mkdir("deps/node");
        
        // copy node.exe to Brackets-node
        grunt.file.copy(exeFile,  "deps/node/Brackets-node.exe");
        
        // unzip NPM
        unzip(npmFile, "deps/node").then(function () {
            nodeWriteVersion();
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: node-mac
    grunt.registerTask("node-mac", "Setup Node.js for Mac OSX and extract", function () {
        // requires node to set "nodeSrc" in config
        grunt.task.requires(["node"]);
        
        var done    = this.async(),
            tarFile = grunt.config("nodeDest")[0],
            tarExec,
            tarName;
        
        grunt.verbose.writeln("Extracting " + tarFile);
        tarExec = exec("tar -xzf " + tarFile);
        
        // remove downloads/, remove .tar.gz extension
        tarName = tarFile.substr(tarFile.lastIndexOf("/") + 1);
        tarName = tarName.substr(0, tarName.lastIndexOf(".tar.gz"));
        
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
    
    // task: node-linux
    grunt.registerTask("node-linux", ["node-mac"]);
    
    // task: node-clean
    grunt.registerTask("node-clean", "Removes Node.js binaries", function () {
        common.deleteFile("deps/node", { force: true });
    });
    
    // task: create-project
    grunt.registerTask("create-project", "Create Xcode/VisualStudio/Makefile project", function () {
        var done = this.async(),
            promise,
            gypCommand;
        
        // TODO why doesn't gyp (in the repository) work for linux?
        if (common.platform() === "linux") {
            gypCommand = "gyp --depth .";
        } else {
            gypCommand = "bash -c 'gyp/gyp appshell.gyp -I common.gypi --depth=.'";
        }
        
        grunt.log.writeln("Building project files");
        
        // Run gyp
        promise = exec(gypCommand);
        
        if (common.platform() === "mac") {
            promise = promise.then(function () {
                // FIXME port to JavaScript?
                return exec("bash scripts/fix-xcode.sh");
            });
        }
        
        promise.then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: setup
    grunt.registerTask("setup", ["cef", "node", "create-project"]);
};
