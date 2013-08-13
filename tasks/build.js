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
/*jslint vars:true, regexp:true, nomen:true*/
/*global module, require, process*/

module.exports = function (grunt) {
    "use strict";
    
    var common      = require("./common")(grunt),
        q           = require("q"),
        spawn       = common.spawn,
        resolve     = common.resolve,
        platform    = common.platform(),
        _           = grunt.util._;
    
    function getBracketsEnv() {
        var env = _.extend(process.env);
        
        env.BRACKETS_SRC = resolve(grunt.config("git.www.repo"));
        env.BRACKETS_APP_NAME = grunt.config("build.name");
        
        return env;
    }
    
    // task: full-build
    grunt.registerTask("full-build", ["git", "create-project", "build", "build-branch", "build-num", "build-sha", "stage", "package"]);
    grunt.registerTask("installer", ["full-build", "build-installer"]);
    
    // task: build
    grunt.registerTask("build", "Build shell executable. Run 'grunt full-build' to update repositories, build the shell and package www files.", function (wwwBranch, shellBranch) {
        grunt.task.run("build-" + platform);
    });
    
    // task: build-mac
    grunt.registerTask("build-mac", "Build mac shell", function () {
        var done = this.async();
        
        spawn([
            "xcodebuild -project appshell.xcodeproj -config Release clean",
            "xcodebuild -project appshell.xcodeproj -config Release build"
        ]).then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: build-win
    grunt.registerTask("build-win", "Build windows shell", function () {
        var done = this.async();
        
        spawn("cmd.exe /c scripts\\build_projects.bat").then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: git
    grunt.registerMultiTask("git", "Pull specified repo branch from origin", function () {
        var repo = this.data.repo;
        
        if (!repo) {
            grunt.fail.fatal("Missing repo config");
        }
        
        repo = resolve(repo);
        
        if (this.data.branch) {
            grunt.log.writeln("Updating repo " + this.target + " at " + repo + " to branch " + this.data.branch);
            
            var done = this.async(),
                promise = spawn([
                    "git fetch origin",
                    "git checkout " + this.data.branch,
                    "git pull origin " + this.data.branch,
                    "git submodule sync",
                    "git submodule update --init --recursive"
                ], { cwd: repo });
        
            promise.then(function () {
                done();
            }, function (err) {
                grunt.log.writeln(err);
                done(false);
            });
        } else {
            grunt.log.writeln("Skipping fetch for " + this.target + " repo");
        }
    });
    
    // task: build-branch
    grunt.registerTask("build-branch", "Write www repo branch to config property build.build-branch", function () {
        var done = this.async(),
            wwwRepo = resolve(grunt.config("git.www.repo"));
        
        spawn(["git status"], { cwd: wwwRepo })
            .then(function (result) {
                var branch = /On branch (.*)/.exec(result.stdout.trim())[1];
                
                grunt.log.writeln("Build branch " + branch);
                grunt.config("build.build-branch", branch);
                
                done();
            }, function (err) {
                grunt.log.error(err);
                done(false);
            });
    });
    
    // task: build-num
    grunt.registerTask("build-num", "Compute www repo build number and set config property build.build-number", function () {
        var done = this.async(),
            wwwRepo = resolve(grunt.config("git.www.repo"));
        
        spawn(["git log --format=%h"], { cwd: wwwRepo })
            .then(function (result) {
                var buildNum = result.stdout.toString().match(/[0-9a-f]\n/g).length;
                
                grunt.log.writeln("Build number " + buildNum);
                grunt.config("build.build-number", buildNum);
                
                done();
            }, function (err) {
                grunt.log.error(err);
                done(false);
            });
    });
    
    // task: build-sha
    grunt.registerTask("build-sha", "Write www repo SHA to config property build.build-sha", function () {
        var done = this.async(),
            wwwRepo = resolve(grunt.config("git.www.repo"));
        
        spawn(["git log -1"], { cwd: wwwRepo })
            .then(function (result) {
                var sha = /commit (.*)/.exec(result.stdout.trim())[1];
                
                grunt.log.writeln("SHA " + sha);
                grunt.config("build.build-sha", sha);
                
                done();
            }, function (err) {
                grunt.log.error(err);
                done(false);
            });
    });
    
    // task: stage
    grunt.registerTask("stage", "Stage release files", function () {
        // stage platform-specific binaries
        grunt.task.run(["clean:staging-" + platform, "stage-" + platform]);
    });
    
    // task: stage-mac
    grunt.registerTask("stage-mac", "Stage mac executable files", function () {
        var done = this.async();
        
        // this should have been a grunt-contrib-copy task "copy:mac", but something goes wrong when creating the .app folder
        spawn([
            "mkdir installer/mac/staging",
            "cp -R xcodebuild/Release/" + grunt.config("build.name") + ".app installer/mac/staging/"
        ]).then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: stage-win
    grunt.registerTask("stage-win", "Stage win executable files", function () {
        // stage platform-specific binaries, then package www files
        grunt.task.run("copy:win");
    });
    
    // task: package
    grunt.registerTask("package", "Package www files", function () {
        grunt.task.run(["clean:www", "copy:www", "copy:samples", "write-config"]);
    });
    
    // task: write-config
    grunt.registerTask("write-config", "Update version data in www config.json payload", function () {
        grunt.task.requires(["build-num", "build-branch", "build-sha"]);
        
        var configPath = grunt.config("build.staging") + "/www/config.json",
            configJSON = grunt.file.readJSON(configPath);
        
        configJSON.version = configJSON.version.substr(0, configJSON.version.lastIndexOf("-") + 1) + grunt.config("build.build-number");
        configJSON.repository.branch = grunt.config("build.build-branch");
        configJSON.repository.SHA = grunt.config("build.build-sha");
        
        common.writeJSON(configPath, configJSON);
    });
    
    // task: installer
    grunt.registerTask("build-installer", "Build installer", function () {
        // TODO update brackets.config.json
        grunt.task.run(["clean:installer-" + platform, "build-installer-" + platform]);
    });
    
    // task: installer-mac
    grunt.registerTask("build-installer-mac", "Build mac installer", function () {
        var done = this.async();
        
        spawn(["bash buildInstaller.sh"], { cwd: resolve("installer/mac"), env: getBracketsEnv() }).then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: installer
    grunt.registerTask("build-installer-win", "Build windows installer", function () {
        var done = this.async();
        
        spawn(["cmd.exe /c ant.bat -f brackets-win-install-build.xml"], { cwd: resolve("installer/win"), env: getBracketsEnv() }).then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
};