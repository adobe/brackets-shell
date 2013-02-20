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
"use strict";

module.exports = function(grunt) {
    
    var xpath           = require("xpath"),
        xmldom          = require("xmldom"),
        guid            = require("guid"),
        domParser       = new xmldom.DOMParser(),
        xmlSerializer   = new xmldom.XMLSerializer();
    
    function writeJSON(path, obj) {
        grunt.file.write(path, JSON.stringify(obj, null, "    "));
    }
    
    function updateXml(path, query, attrName, value) {
        var text    = grunt.file.read(path),
            doc     = domParser.parseFromString(text),
            node    = xpath.select(query, doc)[0];
        
        if (attrName) {
            node.setAttribute(attrName, value);
        } else {
            node.textContent = value;
        }
        
        grunt.file.write(path, xmlSerializer.serializeToString(doc));
    }
    
    function updateXmlAttribute(path, query, attrName, value) {
        updateXml(path, query, attrName, value);
    }
    
    function updateXmlElement(path, query, value) {
        updateXml(path, query, null, value);
    }
    
    // task: set-sprint
    grunt.registerTask("set-sprint", "Update occurrences of sprint number for all native installers and binaries", function () {
        var path            = "package.json",
            packageJSON     = grunt.file.readJSON(path),
            versionShort    = packageJSON.version.substr(0, packageJSON.version.indexOf("-")),
            sprint          = grunt.option("sprint") || 0,
            buildInstallerScriptPath = "installer/mac/buildInstaller.sh",
            buildInstallerScript,
            versionRcPath   = "appshell/version.rc",
            versionRc;
        
        // 1. Update package.json
        packageJSON.version = packageJSON.version.replace(/([0-9]+\.)([0-9]+)(.*)?/, "$1" + sprint + "$3");
        writeJSON(path, packageJSON);
        
        // 2. Open installer/win/brackets-win-install-build.xml and change `product.sprint.number`
        updateXmlAttribute(
            "installer/win/brackets-win-install-build.xml",
            "/project/property[@name='product.sprint.number']",
            "value",
            sprint
        );
        
        // 3. Open installer/mac/buildInstaller.sh and change `releaseName`
        buildInstallerScript = grunt.file.read(buildInstallerScriptPath);
        buildInstallerScript = buildInstallerScript.replace(/(Brackets Sprint )([0-9]+)/, "$1" + sprint);
        grunt.file.write(buildInstallerScriptPath, buildInstallerScript);
        
        // 4. Open installer/win/Brackets.wxs and replace the `StartMenuShortcut` GUID property with a newly generated GUID
        updateXmlAttribute(
            "installer/win/Brackets.wxs",
            "//Component[@Id='StartMenuShortcut']",
            "Guid",
            "{" + guid.raw() + "}"
        );
        
        // 5. Open appshell/version.rc and change `FILEVERSION` and `"FileVersion"`
        versionRc = grunt.file.read(versionRcPath);
        versionRc = versionRc.replace(/(FILEVERSION\s+[0-9]+,)([0-9]+)/, "$1" + sprint);
        versionRc = versionRc.replace(/(Sprint )([0-9]+)/, "$1" + sprint);
        grunt.file.write(versionRcPath, versionRc);
        
        // 6. Open appshell/mac/Info.plist and change `CFBundleShortVersionString` and `CFBundleVersion`
        updateXmlElement(
            "appshell/mac/Info.plist",
            "/plist/dict/key[text()='CFBundleShortVersionString']/following-sibling::string",
            versionShort
        );
        updateXmlElement(
            "appshell/mac/Info.plist",
            "/plist/dict/key[text()='CFBundleVersion']/following-sibling::string",
            versionShort
        );
    });
};