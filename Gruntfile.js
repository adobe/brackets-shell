/*global module, require*/
/*jslint regexp:true*/
module.exports = function (grunt) {
    'use strict';
    
    var xpath           = require('xpath'),
        xmldom          = require('xmldom'),
        guid            = require('guid'),
        domParser       = new xmldom.DOMParser(),
        xmlSerializer   = new xmldom.XMLSerializer();

    // Project configuration.
    grunt.initConfig({
        /* do nothing */
    });
    
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
    grunt.registerTask('set-sprint', function () {
        var path            = "package.json",
            packageJSON     = grunt.file.readJSON(path),
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
        
        // 6. Open appshell/mac/Info.plist and change `CFBundleVersion`
        updateXmlElement(
            "appshell/mac/Info.plist",
            "/plist/dict/key[text()='CFBundleVersion']/following-sibling::string",
            packageJSON.version
        );
    });

    // task: test
    grunt.registerTask('test', ['jshint', 'jasmine']);

    // Default task.
    grunt.registerTask('default', ['test']);
};
