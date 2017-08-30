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
    var common          = require("./common")(grunt),
        fs              = require("fs"),
        path            = require("path"),
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
        exec            = common.exec,
        platform        = common.platform();

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
        return exec("unzip -q \"" + src + "\" -d \"" + dest + "\"");
    }

    // task: cef
    grunt.registerTask("cef", "Download and setup CEF", function () {
        var config   = "cef-" + platform + common.arch(),
            zipSrc   = grunt.config("curl-dir." + config + ".src"),
            zipName  = path.basename(zipSrc),
            zipDest  = path.resolve(process.cwd(), path.join(grunt.config("curl-dir." + config + ".dest"), zipName)),
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



    // task: icu
    grunt.registerTask("icu", "Download and setup ICU", function () {
        var config   = "icu-" + platform + common.arch(),
            zipSrc   = grunt.config("curl-dir." + config + ".src"),
            zipName  = path.basename(zipSrc),
            zipDest  = path.resolve(process.cwd(), path.join(grunt.config("curl-dir." + config + ".dest"), zipName)),
            txtName;

        // extract zip file name and set config property
        grunt.config("icuZipDest", zipDest);
        grunt.config("icuZipSrc", zipSrc);
        grunt.config("icuZipName", zipName);
        grunt.config("icuConfig", config);

        // remove .zip extension
        txtName = zipName.substr(0, zipName.lastIndexOf(".")) + ".txt";

        // optionally download if ICU is not found
        if (!grunt.file.exists("deps/icu/" + txtName)) {
            var icuTasks = ["icu-clean", "icu-extract"];

            if (grunt.file.exists(zipDest)) {
                grunt.verbose.writeln("Found ICU download " + zipDest);
            } else {
                icuTasks.unshift("icu-download");
            }

            grunt.task.run(icuTasks);
        } else {
            grunt.verbose.writeln("Skipping ICU download. Found deps/icu/" + txtName);
        }
    });

    // task: icu-clean
    grunt.registerTask("icu-clean", "Removes CEF binaries and linked folders", function () {
        // delete dev symlinks from "setup_for_hacking"
        common.deleteFile("Release/dev", { force: true });
        common.deleteFile("Debug/dev", { force: true });

        // finally delete ICU binary\
        common.deleteFile("deps/icu", { force: true });
    });
    
    // task: icu-download
    grunt.registerTask("icu-download", "Download ICU, see curl-dir config in Gruntfile.js", function () {
        // requires download-icu to set "icuZipName" in config
        grunt.task.requires(["icu"]);

        // run curl
        grunt.log.writeln("Downloading " + grunt.config("icuZipSrc") + ". This may take a while...");
        grunt.task.run("curl-dir:" + grunt.config("icuConfig"));
    });

    // task: vs-crt
    grunt.registerTask("vs-crt", "Download and setup VS CRT dlls", function () {
        if (platform === "win") {
            var config   = "vs-crt-" + platform + common.arch(),
                zipSrc   = grunt.config("curl-dir." + config + ".src"),
                zipName  = path.basename(zipSrc),
                zipDest  = path.resolve(process.cwd(), path.join(grunt.config("curl-dir." + config + ".dest"), zipName)),
                txtName;

            // extract zip file name and set config property
            grunt.config("vsCRTZipDest", zipDest);
            grunt.config("vsCRTZipSrc", zipSrc);
            grunt.config("vsCRTZipName", zipName);
            grunt.config("vsCRTConfig", config);

            // remove .zip extension
            txtName = zipName.substr(0, zipName.lastIndexOf(".")) + ".txt";

            // optionally download if vs-crt is not found
            if (!grunt.file.exists("deps/vs-crt/" + txtName)) {
                var vsCRTTasks = ["vs-crt-clean", "vs-crt-extract"];

                if (grunt.file.exists(zipDest)) {
                    grunt.verbose.writeln("Found VS CRT download " + zipDest);
                } else {
                    vsCRTTasks.unshift("vs-crt-download");
                }

                grunt.task.run(vsCRTTasks);
            } else {
                grunt.verbose.writeln("Skipping vs-crt download. Found deps/vs-crt/" + txtName);
            }
        }
    });

    // task: vs-crt-clean
    grunt.registerTask("vs-crt-clean", "Removes CEF binaries and linked folders", function () {
        // delete dev symlinks from "setup_for_hacking"
        common.deleteFile("Release/dev", { force: true });
        common.deleteFile("Debug/dev", { force: true });

        // finally delete vs-crt binary\
        common.deleteFile("deps/vs-crt", { force: true });
    });
    
    // task: vs-crt-download
    grunt.registerTask("vs-crt-download", "Download vs crt, see curl-dir config in Gruntfile.js", function () {
        // requires download-vs-crt to set "vsCRTZipName" in config
        grunt.task.requires(["vs-crt"]);

        // run curl
        grunt.log.writeln("Downloading " + grunt.config("vsCRTZipSrc") + ". This may take a while...");
        grunt.task.run("curl-dir:" + grunt.config("vsCRTConfig"));
    });

    function cefFileLocation() {
        return path.resolve(process.cwd(), "deps/cef");
    }

    // Deduct the configuration (debug|release) from the zip file name
    function symbolCompileConfiguration(zipFileName) {
        if (zipFileName) {
            var re = /\w+?[\d+\.\d+\.\d+]+_\w+?_(\w+?)_\w+\.zip/;
            var match = zipFileName.match(re);

            if (!match) {
                grunt.log.error("File name doesn't match the pattern for cef symbols:", zipFileName);
                return zipFileName;
            }

            return match[1];
        } else {
            grunt.log.error("Please provide a zip file name");
        }
    }

    grunt.registerTask("cef-symbols", "Download and unpack the CEF symbols", function () {
        var config      = "cef-" + platform + common.arch() + "-symbols",
            zipSymbols  = grunt.config("curl-dir." + config + ".src");

        if (zipSymbols) {
            var symbolSrcUrls = zipSymbols;

            if (!Array.isArray(zipSymbols)) {
                symbolSrcUrls = [];
                symbolSrcUrls.push(zipSymbols);
            }

            symbolSrcUrls.forEach(function (srcUrl) {
                var zipName = path.basename(srcUrl),
                    zipDest = path.resolve(process.cwd(), path.join(grunt.config("curl-dir." + config + ".dest"), zipName)),
                    txtName,
                    _symbolFileLocation;

                // extract zip file name and set config property
                grunt.config("cefConfig", config);

                // remove .zip extension
                txtName = path.basename(zipName, ".zip") + ".txt";

                // optionally download if CEF is not found
                _symbolFileLocation = path.join(cefFileLocation(), symbolCompileConfiguration(zipName));

                if (!grunt.file.exists(path.resolve(path.join(_symbolFileLocation, txtName)))) {
                    // pass the name of the zip file
                    var zipDestSafe = zipDest.replace(":", "|");
                    var cefTasks = ["cef-symbols-extract" + ":" + zipDestSafe];

                    if (grunt.file.exists(zipDest)) {
                        grunt.verbose.writeln("Found CEF symbols download " + zipDest);
                    } else {
                        cefTasks.unshift("cef-symbols-download" + ":" + config);
                    }

                    grunt.task.run(cefTasks);
                } else {
                    grunt.verbose.writeln("Skipping CEF symbols download. Found " + _symbolFileLocation + "/" + txtName);
                }
            });
        }
    });

    // task: cef-extract-symbols
    grunt.registerTask("cef-symbols-extract", "Extract CEF symbols zip", function () {
        grunt.task.requires(["cef-symbols"]);

        var done    = this.async(),
            zipDest = arguments[0].replace("|", ":"),
            zipName = path.basename(zipDest, '.zip'),
            unzipPromise;

        var symbolFileLocation = path.join(cefFileLocation(), symbolCompileConfiguration(path.basename(zipDest)));

        // unzip to deps/cef
        unzipPromise = unzip(zipDest, symbolFileLocation);

        var symbolDir = path.join(symbolFileLocation, zipName);

        unzipPromise.then(function () {
            var rename = q.denodeify(fs.rename),
                readdir = q.denodeify(fs.readdir);

            return readdir(symbolDir).then(function (files) {
                if (files.length) {
                    var promises = files.map(function (file) {
                        return rename(path.join(symbolDir, file), path.join(path.dirname(symbolDir), file));
                    });

                    return q.all(promises);
                } else {
                    return;
                }
            }, function (err) {
                return err;
            });
        }).then(function () {
            var rmdir = q.denodeify(fs.rmdir);
            return rmdir(symbolDir);
        }).then(function () {
            var memo = path.resolve(path.join(symbolFileLocation, zipName + ".txt"));

            // write empty file with zip file
            grunt.file.write(memo, "");
            done();
        }, function (err) {
            grunt.log.writeln(err);
            done(false);
        });
    });

    // task: cef-download-symbols
    grunt.registerTask("cef-symbols-download", "Download and extract the CEF debug/release symbols", function () {
        grunt.task.requires(["cef-symbols"]);

        var downloadConfig = arguments[0];

        // run curl
        grunt.log.writeln("Downloading " + downloadConfig + ". This may take a while...");
        // curl doesn't give me the option to handle download errors on my own. If the requested file can't
        // be found, curl will log an error to the console.
        grunt.task.run("curl-dir:" + downloadConfig);
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
        zipName = path.basename(zipName, ".zip");

        unzipPromise.then(function () {
            // rename version stamped name to cef
            return rename("deps/" + zipName, "deps/cef");
        }).then(function () {
            var memo = path.resolve(process.cwd(), "deps/cef/" + zipName + ".txt"),
                permissionsPromise;

            if (platform === "mac") {
                // FIXME figure out how to use fs.chmod to only do additive mode u+x
                permissionsPromise = exec("chmod u+x deps/cef/tools/*");
            } else {
                permissionsPromise = q.resolve();
            }

            return permissionsPromise.then(function () {
                // write empty file with zip file
                grunt.file.write(memo, "");

                return q.resolve();
            });
        }).then(function () {
            done();
        }, function (err) {
            grunt.log.writeln(err);
            done(false);
        });
    });


    // task: icu-extract
    grunt.registerTask("icu-extract", "Extract ICU zip", function () {
        // requires icu to set "icuZipName" in config
        grunt.task.requires(["icu"]);

        var done    = this.async(),
            zipDest = grunt.config("icuZipDest"),
            zipName = grunt.config("icuZipName"),
            unzipPromise;

        // unzip to deps/
        unzipPromise = unzip(zipDest, "deps");

        // remove .zip ext
        zipName = path.basename(zipName, ".zip");

        unzipPromise.then(function () {
            // rename version stamped name to cef
            return rename("deps/" + zipName, "deps/icu");
        }).then(function () {
            var memo = path.resolve(process.cwd(), "deps/icu/" + zipName + ".txt"),
                permissionsPromise;
            
            permissionsPromise = q.resolve();

            return permissionsPromise.then(function () {
                // write empty file with zip file
                grunt.file.write(memo, "");

                return q.resolve();
            });
        }).then(function () {
            done();
        }, function (err) {
            grunt.log.writeln(err);
            done(false);
        });
    });

    // task: vs-crt-extract
    grunt.registerTask("vs-crt-extract", "Extract vs-crt zip", function () {
        // requires vs-crt to set "vsCRTZipName" in config
        grunt.task.requires(["vs-crt"]);

        var done    = this.async(),
            zipDest = grunt.config("vsCRTZipDest"),
            zipName = grunt.config("vsCRTZipName"),
            unzipPromise;

        // unzip to deps/
        unzipPromise = unzip(zipDest, "deps");

        // remove .zip ext
        zipName = path.basename(zipName, ".zip");

        unzipPromise.then(function () {
            // rename version stamped name to cef
            return rename("deps/" + zipName, "deps/vs-crt");
        }).then(function () {
            var memo = path.resolve(process.cwd(), "deps/vs-crt/" + zipName + ".txt"),
                permissionsPromise;
            
            permissionsPromise = q.resolve();

            return permissionsPromise.then(function () {
                // write empty file with zip file
                grunt.file.write(memo, "");

                return q.resolve();
            });
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
        var config      = "node-" + platform + common.arch(),
            nodeSrc     = grunt.config("curl-dir." + config + ".src"),
            nodeDest    = [],
            dest        = grunt.config("curl-dir." + config + ".dest"),
            curlTask    = "curl-dir:" + config,
            setupTask   = "node-" + platform,
            nodeVersion = grunt.config("node.version"),
            txtName     = "version-" + nodeVersion + ".txt",
            missingDest = false;

        // extract file name and set config property
        nodeSrc = Array.isArray(nodeSrc) ? nodeSrc : [nodeSrc];

        // curl-dir:node-win32 defines multiple downloads, account for this array
        nodeSrc.forEach(function (value, index) {
            nodeSrc[index] = value.substr(value.lastIndexOf("/") + 1);
            nodeDest[index] = path.join(dest, nodeSrc[index]);

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

    grunt.registerTask("node-check", function() {
        var done = this.async(),
            promise,
            systemNodeCheck,
            bundledNodeCheck,
            bundledNodeLocation,
            bundledNodeVersion,
            systemNodeVersion;

        if (platform === "win") {
            bundledNodeLocation = path.join("deps", "node", "node.exe");
        }
        else {
            bundledNodeLocation = path.join("deps", "node", "bin", "Brackets-node");
        }
        

        bundledNodeCheck = exec(bundledNodeLocation + " -v");

        bundledNodeCheck.then(function (bundleNodeCmdStdout) {
            bundledNodeVersion = bundleNodeCmdStdout[0];
        }).then(function() {
            systemNodeCheck = exec("node -v");
            return systemNodeCheck;
        })
        .then(function(systemNodeCmdStdout) {
            systemNodeVersion = systemNodeCmdStdout[0];
            return systemNodeVersion === bundledNodeVersion ? done() : done(false);
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });

    function nodeWriteVersion() {
        // write empty file with node-version name
        grunt.file.write("deps/node/version-" + grunt.config("node.version") + ".txt", "");
    }

    // task: node-win32
    grunt.registerTask("node-win", "Setup Node.js for Windows", function () {
        // requires node to set "nodeSrc" in config
        grunt.task.requires(["node"]);

        var nodeDest    = grunt.config("nodeDest"),
            exeFile     = nodeDest;

        grunt.file.mkdir("deps/node");

        // copy node.exe to Brackets-node
        grunt.file.copy(exeFile,  "deps/node/node.exe");

        nodeWriteVersion();
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

        if (platform === "linux") {
            gypCommand = "bash -c 'gyp/gyp --depth=.'";
        } else {
            gypCommand = "bash -c 'gyp/gyp appshell.gyp -I common.gypi --depth=.'";
        }

        grunt.log.writeln("Building project files");

        // Run gyp
        promise = exec(gypCommand);

        if (platform === "mac") {
            promise = promise.then(function () {
                // FIXME port to JavaScript?
                return exec("bash scripts/fix-xcode.sh");
            });
        } else if (platform === "win") {
            promise = promise.then(function () {
                // FIXME port to JavaScript?
                return exec("bash scripts/fix-msvc.sh");
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
    if (platform === "win") {
        grunt.registerTask("setup", ["cef", "node", "node-check", "icu", "vs-crt", "create-project"]);
    } else {
        grunt.registerTask("setup", ["cef", "node", "node-check", "icu", "create-project"]);
    }
};