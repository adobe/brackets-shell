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

    var common = require("./common")(grunt),
        semver = require("semver");

    function safeReplace(content, regexp, replacement) {
        if (! regexp.test(content)) {
            grunt.fail.fatal("Regexp " + regexp + " did not match");
        }

        var newContent = content.replace(regexp, replacement);

        if (newContent === content) {
            grunt.log.warn("Buildnumber hasn't been updated, because it's already set.");
        }

        return newContent;
    }

    // task: set-release
    grunt.registerTask("set-release", "Update occurrences of release number for all native installers and binaries", function () {
        var packageJsonPath             = "package.json",
            packageJSON                 = grunt.file.readJSON(packageJsonPath),
            winInstallerBuildXmlPath    = "installer/win/brackets-win-install-build.xml",
            buildInstallerScriptPath    = "installer/mac/buildInstaller.sh",
            wxsPath                     = "installer/win/Brackets.wxs",
            versionRcPath               = "appshell/version.rc",
            infoPlistPath               = "appshell/mac/Info.plist",
            release                     = grunt.option("release") || "",
            text;

        if (!release || !semver.valid(release)) {
            grunt.fail.fatal("Please specify a release. e.g. grunt set-release --release=1.0.0");
        }

        var newVersion = semver.parse(release);

        // 1. Update package.json
        packageJSON.version = newVersion.version + "-0";
        common.writeJSON(packageJsonPath, packageJSON);

        // 2. Open installer/win/brackets-win-install-build.xml and change `product.release.number`
        text = grunt.file.read(winInstallerBuildXmlPath);
        text = safeReplace(
            text,
            /<property name="product\.release\.number" value="(.*)"\/>/,
            '<property name="product.release.number" value="' + newVersion.version + '"/>'
        );
        grunt.file.write(winInstallerBuildXmlPath, text);

        // 3. Open installer/mac/buildInstaller.sh and change `dmgName`
        text = grunt.file.read(buildInstallerScriptPath);
        text = safeReplace(
            text,
            /(version)=".*"/,
            "$1" + "=\"" + newVersion.version + "\""
        );
        grunt.file.write(buildInstallerScriptPath, text);

        // 4. Open appshell/version.rc and change `FILEVERSION` and `"FileVersion"`
        text = grunt.file.read(versionRcPath);
        text = safeReplace(
            text,
            /(FILEVERSION\s+).*/,
            "$1" + newVersion.major + "," + newVersion.minor + "," + newVersion.patch + "," + (newVersion.build.length ? newVersion.build : "0")
        );
        text = safeReplace(
            text,
            /(Release )([\d+.]+)/,
            "$1" + newVersion.version
        );
        grunt.file.write(versionRcPath, text);

        // 5. Open appshell/mac/Info.plist and change `CFBundleShortVersionString` and `CFBundleVersion`
        text = grunt.file.read(infoPlistPath);
        text = safeReplace(
            text,
            /(<key>CFBundleVersion<\/key>\s*<string>)([0-9]+\.[0-9]+\.[0-9]+)(<\/string>)/,
            "$1" + newVersion.version + "$3"
        );
        text = safeReplace(
            text,
            /(<key>CFBundleShortVersionString<\/key>\s*<string>)([0-9]+\.[0-9]+\.[0-9]+)(<\/string>)/,
            "$1" + newVersion.version + "$3"
        );
        grunt.file.write(infoPlistPath, text);
    });
};
