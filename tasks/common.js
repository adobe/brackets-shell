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
/*jslint es5:true, vars: true, plusplus: true, regexp: true, nomen: true*/
/*global module, require, process, Buffer*/
module.exports = function (grunt) {
    "use strict";
    
    var q               = require("q"),
        fs              = require("fs"),
        child_process   = require("child_process"),
        path            = require("path"),
        common          = {},
        qexec           = q.denodeify(child_process.exec),
        _platform;
    
    var SPLIT_ARGS      = /\S+|"[^"]+"|'[^']+'/g;
    
    function exec(cmd) {
        return qexec(cmd);
    }
    
    /**
     * Spawns async commands in parallel, piping each command's output to the next.
     * @param {!Array.<string>} commands
     * @param {Object?} opts child_process.spawn options
     * @returns {Promise} resolved with {{code: number, stdout: string, stderr: string}}
     */
    function pipe(commands, opts) {
        var done = grunt.task.current.async(),
            promises = [],
            children = [];
        
        opts = opts || {};
        opts.env = opts.env || process.env;
        opts.cwd = opts.cwd || process.cwd();
        
        // spawn commands simultaneously
        commands.forEach(function (command) {
            var deferred = q.defer(),
                indexOf = command.indexOf(" "),
                args = (indexOf > 0) ? command.substr(indexOf + 1).match(SPLIT_ARGS) : [],
                cmd = (indexOf < 0) ? command : command.substr(0, indexOf),
                child,
                stdout = new Buffer(""),
                stderr = new Buffer("");
            
            grunt.verbose.ok(command);
            
            child = child_process.spawn(cmd, args, opts);
            children.push(child);
            
            if (child.stdout) {
                child.stdout.on("data", function (buf) {
                    stdout = Buffer.concat([stdout, new Buffer(buf)]);
                });
            }
            if (child.stderr) {
                child.stderr.on("data", function (buf) {
                    stderr = Buffer.concat([stderr, new Buffer(buf)]);
                    grunt.verbose.error(command + ":" + buf.toString());
                });
            }
            child.on("close", function (code) {
                var closeResult = {
                    code: code,
                    stdout: stdout.toString(),
                    stderr: stderr.toString()
                };
                
                if (code !== 0) {
                    grunt.verbose.error("Error code: " + code);
                    deferred.reject(closeResult);
                } else {
                    deferred.resolve(closeResult);
                }
            });
            
            promises.push(deferred.promise);
        });
        
        var makePipeCallback = function (next) {
                return function (buf) { next.stdin.write(buf); };
            },
            makeCloseCallback = function (next) {
                return function () { next.stdin.end(); };
            };
        
        // pipe
        var i, current, next;
        for (i = 0; i < commands.length - 1; i++) {
            current = children[i];
            next = children[i + 1];
            
            current.stdout.on("data", makePipeCallback(next));
            current.stdout.on("close", makeCloseCallback(next));
        }
        
        var result = q.defer();
        
        q.all(promises).then(function () {
            done();
        }, function (output) {
            grunt.log.error(output.stderr);
            done(false);
        });
        
        // get the output of the last process
        promises[promises.length - 1].then(function (output) {
            grunt.verbose.ok();
            result.resolve(output);
        });
        
        return result.promise;
    }
    
    /**
     * Spawns async commands in sequence
     * @param {!(string|Array.<string>)} commands
     * @param {Object?} opts child_process.spawn options
     * @returns {Promise} resolved with {{code: number, stdout: string, stderr: string}}
     */
    function spawn(commands, opts) {
        var result = q.resolve(),
            children = [],
            i,
            current,
            next;
        
        opts = opts || {};
        opts.env = opts.env || process.env;
        opts.cwd = opts.cwd || process.cwd();
        commands = Array.isArray(commands) ? commands : [commands];
        
        if (opts.cwd) {
            grunt.verbose.ok(opts.cwd);
        }
        
        commands.forEach(function (command) {
            result = result.then(function () {
                var deferred = q.defer(),
                    indexOf = command.indexOf(" "),
                    cmdArgs = (indexOf > 0) ? command.substr(indexOf + 1).match(SPLIT_ARGS) : [],
                    args = opts.args || cmdArgs,
                    cmd = (indexOf < 0) ? command : command.substr(0, indexOf),
                    child,
                    stdout = new Buffer(""),
                    stderr = new Buffer("");
                
                grunt.verbose.ok(command);
                
                child = child_process.spawn(cmd, args, opts);
                children.push(child);
                
                if (child.stdout) {
                    child.stdout.on("data", function (buf) {
                        stdout = Buffer.concat([stdout, new Buffer(buf)]);
                        grunt.verbose.ok(buf.toString());
                    });
                }
                if (child.stderr) {
                    child.stderr.on("data", function (buf) {
                        stderr = Buffer.concat([stderr, new Buffer(buf)]);
                        grunt.verbose.error(buf.toString());
                    });
                }
                child.on("close", function (code) {
                    var closeResult = {
                        cmd: cmd,
                        args: args,
                        code: code,
                        stdout: stdout.toString(),
                        stderr: stderr.toString(),
                        toString: function () {
                            if (code === 0) {
                                return this.stdout;
                            } else {
                                if (stderr.length > 0) {
                                    return this.stderr;
                                } else {
                                    return this.stdout;
                                }
                            }
                        }
                    };
                    
                    if (code !== 0) {
                        grunt.verbose.error("Error code: " + code);
                        deferred.reject(closeResult);
                    } else {
                        grunt.verbose.ok();
                        deferred.resolve(closeResult);
                    }
                });
                
                return deferred.promise;
            });
        });
        
        return result;
    }
    
    function resolve(relPath) {
        return path.resolve(process.cwd(), relPath);
    }
    
    function platform() {
        if (!_platform) {
            if (process.platform === "darwin") {
                _platform = "mac";
            } else if (process.platform === "win32") {
                _platform = "win";
            } else {
                _platform = "linux";
            }
        }

        return _platform;
    }
    
    function arch() {
        if (platform() === "linux") {
            if (process.arch === "x64") {
                return 64;
            } else {
                return 32;
            }
        }
        
        return "";
    }
    
    function deleteFile(path, options) {
        if (grunt.file.exists(path)) {
            grunt.file.delete(path, options);
        }
    }
    
    function writeJSON(path, obj) {
        grunt.file.write(path, JSON.stringify(obj, null, "    "));
    }
    
    common.exec = exec;
    common.pipe = pipe;
    common.spawn = spawn;
    common.resolve = resolve;
    common.platform = platform;
    common.arch = arch;
    common.deleteFile = deleteFile;
    common.writeJSON = writeJSON;
    common.rename = q.denodeify(fs.rename);
    
    return common;
};
