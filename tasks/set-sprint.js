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
/*global module, require*/

module.exports = function (grunt) {
    "use strict";
    
    var common = require("./common")(grunt);

    function safeReplace(content, regexp, replacement) {
        var newContent = content.replace(regexp, replacement);

        if (newContent === content) {
            grunt.fail.fatal("Regexp " + regexp + " did not match");
        }

        return newContent;
    }
    
    // task: set-sprint
    grunt.registerTask("set-sprint", "Update occurrences of sprint number for all native installers and binaries", function () {
        var packageJsonPath             = "package.json",
            packageJSON                 = grunt.file.readJSON(packageJsonPath),
            winInstallerBuildXmlPath    = "installer/win/brackets-win-install-build.xml",
            buildInstallerScriptPath    = "installer/mac/buildInstaller.sh",
            wxsPath                     = "installer/win/Brackets.wxs",
            versionRcPath               = "appshell/version.rc",
            infoPlistPath               = "appshell/mac/Info.plist",
            sprint                      = grunt.option("sprint") || 0,
            versionShort                = packageJSON.version.substr(0, packageJSON.version.indexOf("-")),
            text;

        // replace sprint number in short version, e.g. N.<sprint>.N
        versionShort = versionShort.replace(/([0-9]+\.)([0-9]+)(\.[0-9]+)/, "$1" + sprint + "$3");

        if (!sprint) {
            grunt.fail.fatal("Please specify a sprint. e.g. grunt set-sprint --sprint=21");
        }
        
        // 1. Update package.json
        packageJSON.version = safeReplace(
            packageJSON.version,
            /([0-9]+\.)([0-9]+)([\.\-a-zA-Z0-9]*)?/,
            "$1" + sprint + "$3"
        );
        common.writeJSON(packageJsonPath, packageJSON);
        
        // 2. Open installer/win/brackets-win-install-build.xml and change `product.sprint.number`
        text = grunt.file.read(winInstallerBuildXmlPath);
        text = safeReplace(
            text,
            /<property name="product\.sprint\.number" value="([0-9]+)"\/>/,
            '<property name="product.sprint.number" value="' + sprint + '"/>'
        );
        grunt.file.write(winInstallerBuildXmlPath, text);
        
        // 3. Open installer/mac/buildInstaller.sh and change `releaseName`
        text = grunt.file.read(buildInstallerScriptPath);
        text = safeReplace(
            text,
            /(Brackets Sprint )([0-9]+)/,
            "$1" + sprint
        );
        grunt.file.write(buildInstallerScriptPath, text);
        
        // 4. Open appshell/version.rc and change `FILEVERSION` and `"FileVersion"`
        text = grunt.file.read(versionRcPath);
        text = safeReplace(
            text,
            /(FILEVERSION\s+[0-9]+,)([0-9]+)/,
            "$1" + sprint
        );
        text = safeReplace(
            text,
            /(Sprint )([0-9]+)/,
            "$1" + sprint
        );
        grunt.file.write(versionRcPath, text);
        
        // 5. Open appshell/mac/Info.plist and change `CFBundleShortVersionString` and `CFBundleVersion`text = grunt.file.read(wxsPath);
        text = grunt.file.read(infoPlistPath);
        text = safeReplace(
            text,
            /(<key>CFBundleVersion<\/key>\s*<string>)([0-9]+\.[0-9]+\.[0-9]+)(<\/string>)/,
            "$1" + versionShort + "$3"
        );
        text = safeReplace(
            text,
            /(<key>CFBundleShortVersionString<\/key>\s*<string>)([0-9]+\.[0-9]+\.[0-9]+)(<\/string>)/,
            "$1" + versionShort + "$3"
        );
        grunt.file.write(infoPlistPath, text);
    });
};