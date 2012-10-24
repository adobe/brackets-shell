/*
 * Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
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

// This is the JavaScript code for bridging to native functionality
// See appshell_extentions_[platform] for implementation of native methods.
//
// Note: All file native file i/o functions are synchronous, but are exposed
// here as asynchronous calls. 

/*jslint vars: true, plusplus: true, devel: true, browser: true, nomen: true, indent: 4, forin: true, maxerr: 50, regexp: true */
/*global define, native */

var appshell;
if (!appshell) {
    appshell = {};
}
if (!appshell.fs) {
    appshell.fs = {};
}
if (!appshell.app) {
    appshell.app = {};
}
(function () {    
    // Error values. These MUST be in sync with the error values
    // at the top of appshell_extensions_platform.h.
    
    /**
     * @constant No error.
     */
    appshell.fs.NO_ERROR                    = 0;
    
    /**
     * @constant Unknown error occurred.
     */
    appshell.fs.ERR_UNKNOWN                 = 1;
    
    /**
     * @constant Invalid parameters passed to function.
     */
    appshell.fs.ERR_INVALID_PARAMS          = 2;
    
    /**
     * @constant File or directory was not found.
     */
    appshell.fs.ERR_NOT_FOUND               = 3;
    
    /**
     * @constant File or directory could not be read.
     */
    appshell.fs.ERR_CANT_READ               = 4;
    
    /**
     * @constant An unsupported encoding value was specified.
     */
    appshell.fs.ERR_UNSUPPORTED_ENCODING    = 5;
    
    /**
     * @constant File could not be written.
     */
    appshell.fs.ERR_CANT_WRITE              = 6;
    
    /**
     * @constant Target directory is out of space. File could not be written.
     */
    appshell.fs.ERR_OUT_OF_SPACE            = 7;
    
    /**
     * @constant Specified path does not point to a file.
     */
    appshell.fs.ERR_NOT_FILE                = 8;
    
    /**
     * @constant Specified path does not point to a directory.
     */
    appshell.fs.ERR_NOT_DIRECTORY           = 9;
 
    /**
     * @constant Specified file already exists.
     */
    appshell.fs.ERR_FILE_EXISTS             = 10;
 
    /**
     * Display the OS File Open dialog, allowing the user to select
     * files or directories.
     *
     * @param {boolean} allowMultipleSelection If true, multiple files/directories can be selected.
     * @param {boolean} chooseDirectory If true, only directories can be selected. If false, only 
     *        files can be selected.
     * @param {string} title Tile of the open dialog.
     * @param {string} initialPath Initial path to display in the dialog. Pass NULL or "" to 
     *        display the last path chosen.
     * @param {Array.<string>} fileTypes Array of strings specifying the selectable file extensions. 
     *        These strings should not contain '.'. This parameter is ignored when 
     *        chooseDirectory=true.
     * @param {function(err, selection)} callback Asynchronous callback function. The callback gets two arguments 
     *        (err, selection) where selection is an array of the names of the selected files.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function ShowOpenDialog();
    appshell.fs.showOpenDialog = function (allowMultipleSelection, chooseDirectory, title, initialPath, fileTypes, callback) {
        setTimeout(function () {
            ShowOpenDialog(callback, allowMultipleSelection, chooseDirectory,
                             title || 'Open', initialPath || '',
                             fileTypes ? fileTypes.join(' ') : '');
        }, 10);
    };
    
    /**
     * Reads the contents of a directory. 
     *
     * @param {string} path The path of the directory to read.
     * @param {function(err, files)} callback Asynchronous callback function. The callback gets two arguments 
     *        (err, files) where files is an array of the names of the files
     *        in the directory excluding '.' and '..'.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_UNKNOWN
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *          ERR_CANT_READ
     *                 
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function ReadDir();
    appshell.fs.readdir = function (path, callback) {
        var resultString = ReadDir(callback, path);
    };
     
    /**
     * Create a new directory.
     *
     * @param {string} path The path of the directory to create.
     * @param {number} mode The permissions for the directory, in numeric format (ie 0777)
     * @param {function(err)} callback Asynchronous callback function. The callback gets one argument.
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     **/
    native function MakeDir();
    appshell.fs.makedir = function (path, mode, callback) {
        MakeDir(callback, path, mode);
    };

    /**
     * Rename a file or directory.
     *
     * @param {string} oldPath The old name of the file or directory.
     * @param {string} newPath The new name of the file or directory.
     * @param {function(err)} callback Asynchronous callback function. The callback gets one argument.
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     **/
    native function Rename();
    appshell.fs.rename = function(oldPath, newPath, callback) {
        Rename(callback, oldPath, newPath);
    };
 
    /**
     * Get information for the selected file or directory.
     *
     * @param {string} path The path of the file or directory to read.
     * @param {function(err, stats)} callback Asynchronous callback function. The callback gets two arguments 
     *        (err, stats) where stats is an object with isFile() and isDirectory() functions.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_UNKNOWN
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *                 
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function GetFileModificationTime();
    appshell.fs.stat = function (path, callback) {
        GetFileModificationTime(function (err, modtime, isDir) {
            callback(err, {
                isFile: function () {
                    return !isDir;
                },
                isDirectory: function () {
                    return isDir;
                },
                mtime: new Date(modtime * 1000) // modtime is seconds since 1970, convert to ms
            });
        }, path);
    };
 
    /**
     * Quits native shell application
     */
    native function QuitApplication();
    appshell.app.quit = function () {
        QuitApplication();
    };
 
    /**
     * Abort a quit operation
     */
    native function AbortQuit();
    appshell.app.abortQuit = function () {
        AbortQuit();
    };

    /**
     * Invokes developer tools application
     */
    native function ShowDeveloperTools();
    appshell.app.showDeveloperTools = function () {
        ShowDeveloperTools();
    };

    /**
     * Reads the entire contents of a file. 
     *
     * @param {string} path The path of the file to read.
     * @param {string} encoding The encoding for the file. The only supported encoding is 'utf8'.
     * @param {function(err, data)} callback Asynchronous callback function. The callback gets two arguments 
     *        (err, data) where data is the contents of the file.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_UNKNOWN
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *          ERR_CANT_READ
     *          ERR_UNSUPPORTED_ENCODING
     *                 
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function ReadFile();
    appshell.fs.readFile = function (path, encoding, callback) {
        ReadFile(callback, path, encoding);
    };
    
    /**
     * Write data to a file, replacing the file if it already exists. 
     *
     * @param {string} path The path of the file to write.
     * @param {string} data The data to write to the file.
     * @param {string} encoding The encoding for the file. The only supported encoding is 'utf8'.
     * @param {function(err)} callback Asynchronous callback function. The callback gets one argument (err).
     *        Possible error values:
     *          NO_ERROR
     *          ERR_UNKNOWN
     *          ERR_INVALID_PARAMS
     *          ERR_UNSUPPORTED_ENCODING
     *          ERR_CANT_WRITE
     *          ERR_OUT_OF_SPACE
     *                 
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function WriteFile();
    appshell.fs.writeFile = function (path, data, encoding, callback) {
        WriteFile(callback, path, data, encoding);
    };
    
    /**
     * Set permissions for a file or directory.
     *
     * @param {string} path The path of the file or directory
     * @param {number} mode The permissions for the file or directory, in numeric format (ie 0777)
     * @param {function(err)} callback Asynchronous callback function. The callback gets one argument (err).
     *        Possible error values:
     *          NO_ERROR
     *          ERR_UNKNOWN
     *          ERR_INVALID_PARAMS
     *          ERR_CANT_WRITE
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function SetPosixPermissions();
    appshell.fs.chmod = function (path, mode, callback) {
        SetPosixPermissions(callback, path, mode);
    };
    
    /**
     * Delete a file.
     *
     * @param {string} path The path of the file to delete
     * @param {function(err)} callback Asynchronous callback function. The callback gets one argument (err).
     *        Possible error values:
     *          NO_ERROR
     *          ERR_UNKNOWN
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *          ERR_NOT_FILE
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function DeleteFileOrDirectory();
    appshell.fs.unlink = function (path, callback) {
        DeleteFileOrDirectory(callback, path);
    };

    /**
     * Return the number of milliseconds that have elapsed since the application
     * was launched. 
     */
    native function GetElapsedMilliseconds();
    appshell.app.getElapsedMilliseconds = function () {
        return GetElapsedMilliseconds();
    }
    
    /**
     * Open the live browser
     *
     * @param {string} url
     * @param {boolean} enableRemoteDebugging
     * @param {function(err)} callback Asynchronous callback function with one argument (the error)
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS - invalid parameters
     *          ERR_UNKNOWN - unable to launch the browser
     *          ERR_NOT_FOUND - unable to find a browers to launch
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function OpenLiveBrowser();
    appshell.app.openLiveBrowser = function (url, enableRemoteDebugging, callback) {
        // enableRemoteDebugging flag is ignored on mac
        setTimeout(function() {
            OpenLiveBrowser(callback, url, enableRemoteDebugging);
        }, 0);
    };
    
    /**
     * Attempts to close the live browser. The browser can still give the user a chance to override
     * the close attempt if there is a page with unsaved changes. This function will fire the
     * callback when the browser is closed (No_ERROR) or after a three minute timeout (ERR_UNKNOWN). 
     *
     * @param {function(err)} callback Asynchronous callback function with one argument (the error)
     *        Possible error values:
     *          NO_ERROR (all windows are closed by the time the callback is fired)
     *          ERR_UNKNOWN - windows are currently open, though the user may be getting prompted by the 
     *                      browser to close them
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function CloseLiveBrowser();
    appshell.app.closeLiveBrowser = function (callback) {
        CloseLiveBrowser(callback);
    };
 
    /**
     * Open a URL in the default OS browser window. 
     *
     * @param {function(err)} callback Asynchronous callback function with one argument (the error)
     * @param {string} url URL to open in the browser.
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function OpenURLInDefaultBrowser();
    appshell.app.openURLInDefaultBrowser = function (callback, url) {
        OpenURLInDefaultBrowser(callback, url);
    };
 
    /**
     * Return the user's language per operating system preferences.
     */
    native function GetCurrentLanguage();
    Object.defineProperty(appshell.app, "language", {
        writeable: false,
        get : function() { return GetCurrentLanguage(); },
        enumerable : true,
        configurable : false
    });
 
    /**
     * Returns the full path of the application support directory.
     * On the Mac, it's /Users/<user>/Library/Application Support/GROUP_NAME/APP_NAME
     * On Windows, it's C:\Users\<user>\AppData\Roaming\GROUP_NAME\APP_NAME
     *
     * @return {string} Full path of the application support directory
     */
    native function GetApplicationSupportDirectory();
    appshell.app.getApplicationSupportDirectory = function () {
        return GetApplicationSupportDirectory();
    }
  
    /**
     * Open the specified folder in an OS file window.
     *
     * @param {string} path Path of the folder to show.
     * @param {function(err)} callback Asyncrhonous callback function with one argument (the error)
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function ShowOSFolder();
    appshell.app.showOSFolder = function (path, callback) {
        ShowOSFolder(callback, path);
    }
 
    /**
     * Open the extensions folder in an OS file window.
     *
     * @param {string} appURL URL of the index.html file for the application
     * @param {function(err)} callback Asynchronous callback function with one argument (the error)
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    appshell.app.showExtensionsFolder = function (appURL, callback) {
        appshell.app.showOSFolder(GetApplicationSupportDirectory() + "/extensions", callback);
    };
 
    // Alias the appshell object to brackets. This is temporary and should be removed.
    brackets = appshell;
})();
