/*
 * Copyright (c) 2012 - present Adobe Systems Incorporated. All rights reserved.
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
     * @constant File could not be encoded.
     */
    appshell.fs.ERR_ENCODE_FILE_FAILED      = 18;

    /**
     * @constant File could not be decoded.
     */
    appshell.fs.ERR_DECODE_FILE_FAILED      = 19;

    /**
     * @constant File was encoded with utf-16
     */
    appshell.fs.ERR_UNSUPPORTED_UTF16_ENCODING      = 20;
    
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
     * @constant The required browser is not installed
     */
    appshell.fs.ERR_BROWSER_NOT_INSTALLED   = 11;
 
    /**
     * @constant User cancelled the password dialog while installing command line tools.
     */
    appshell.app.ERR_CL_TOOLS_CANCELLED     = 12;
    
    /**
     * @constant Removing and existing sysmlink failed
     */
    appshell.app.ERR_CL_TOOLS_RMFAILED      = 13;
    
    /**
     * @constant Directory, into which symlink needs to be copied, creation failed
     */
    appshell.app.ERR_CL_TOOLS_MKDIRFAILED   = 14;
    
    /**
     * @constant Symlink creation failed
     */
    appshell.app.ERR_CL_TOOLS_SYMLINKFAILED = 15;
    
    /**
     * @constant Brackets could not create an Authorization object
     */
    appshell.app.ERR_CL_TOOLS_SERVFAILED    = 16;

    /**
    * @constant Command Line Installation not supported on the specific platform
    */
    appshell.app.ERR_CL_TOOLS_NOTSUPPORTED  = 17;
 
    /**
     * @constant No error.
     */
    appshell.app.NO_ERROR                   = 0;

    /**
     * @constant Node has not yet launched. Try again later.
     */
    appshell.app.ERR_NODE_NOT_YET_STARTED   = -1;

    /**
     * @constant Node is in the process of launching, but has not yet set the port.
     * Try again later.
     */
    appshell.app.ERR_NODE_PORT_NOT_YET_SET  = -2;

    /**
     * @constant Node encountered a fatal error while launching or running.
     * It cannot be restarted.
     */
    appshell.app.ERR_NODE_FAILED            = -3;

    /*
     * @private 
     */
    var _dummyCallback = function () {
    };

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
      * Display the OS Save dialog, allowing the user to provide a path and name to save a file.
      *
      * @param {string} title Title of the Save As dialog.
      * @param {string} initialPath Initial path to display in the dialog. Pass NULL or "" to
      *        display the last path chosen.
      * @param {function(err, selection)} callback Asynchronous callback function. The callback gets two arguments
      *        (err, absolutefilepath) where absolutefilepath is a string with the absolute path to the file.
      *        Possible error values:
      *          NO_ERROR
      *          ERR_INVALID_PARAMS
      *
      * @return None. This is an asynchronous call that sends all return information to the callback.
      */
     native function ShowSaveDialog();
     appshell.fs.showSaveDialog = function (title, initialPath, proposedNewFilename, callback) {
         setTimeout(function () {
             ShowSaveDialog(callback,
                            title || 'Save As', initialPath || '', proposedNewFilename || '');
         }, 10);
     };

    /**
     * Check whether a given path is from a network drive. 
     *
     * @param {string} path The path to check for a network drive.
     * @param {function(err, isRemote)} callback Asynchronous callback function. The callback gets two arguments 
     *        (err, isRemote) where isRemote indicates whether the given path is a mapped network drive or not.
     * 
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function IsNetworkDrive();
    appshell.fs.isNetworkDrive = function (path, callback) {
        var resultString = IsNetworkDrive(callback, path);
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
     * @param {function(err)=}  callback Asynchronous callback function. The callback gets one argument.
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     **/
    native function MakeDir();
    appshell.fs.makedir = function (path, mode, callback) {
        // RFC: mode should be 0777 if it's undefined
        MakeDir(callback || _dummyCallback, path, mode);
    };

    /**
     * Rename a file or directory.
     *
     * @param {string} oldPath The old name of the file or directory.
     * @param {string} newPath The new name of the file or directory.
     * @param {function(err)=} callback Asynchronous callback function. The callback gets one argument.
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     **/
    native function Rename();
    appshell.fs.rename = function(oldPath, newPath, callback) {
        Rename(callback || _dummyCallback, oldPath, newPath);
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
    native function GetFileInfo();
    appshell.fs.stat = function (path, callback) {
        GetFileInfo(function (err, modtime, isDir, size, realPath) {
            callback(err, {
                isFile: function () {
                    return !isDir;
                },
                isDirectory: function () {
                    return isDir;
                },
                mtime: new Date(modtime * 1000), // modtime is seconds since 1970, convert to ms
                size: new Number(size),
                realPath: realPath ? realPath : null
            });
        }, path);
    };
 
    /**
     * Reads the contents of a directory and reports contents along with stats. 
     *
     * @param {string} path The path of the directory to read.
     * @param {function(err, files)} callback Asynchronous callback function. The callback gets three arguments 
     *        (err, files, stats) where files is an array of the names of the files
     *        in the directory excluding '.' and '..'  and stats is an array of all stats of the files.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_UNKNOWN
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *          ERR_CANT_READ
     *                 
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */ 
    // Test dictionary
    native function ReadDirWithStats();
    appshell.fs.readDirWithStats = function (path, callback){

        ReadDirWithStats(function (err, allPaths){
            if (callback) {
                var finalArray  = [];
                var allContents = allPaths[0];
                var allStats    = allPaths[1];

                allStats.forEach(function (val, idx) {
                    finalArray[idx] = {
                        isFile: function () {
                            return !val[1];
                        },
                        isDirectory: function () {
                            return val[1];
                        },
                        mtime: new Date(val[0] * 1000), // modtime is seconds since 1970, convert to ms
                        size: new Number(val[2]),
                        realPath: val[3] ? val[3] : null
                    }
                });
                callback(err, allContents, finalArray);
            }
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
     * Returns the TCP port of the current Node server 
     *
     * @param {function(err, port)} callback Asynchronous callback function. The callback gets two arguments 
     *        (err, port) where port is the TCP port of the running server.
     *        Possible error values:
     *         ERR_NODE_PORT_NOT_SET      = -1;
     *         ERR_NODE_NOT_RUNNING       = -2;
     *         ERR_NODE_FAILED            = -3;
     *                 
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function GetNodeState();
    appshell.app.getNodeState = function (callback) {
        GetNodeState(callback);
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
     * @param {function(err)=} callback Asynchronous callback function. The callback gets one argument (err).
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
    appshell.fs.writeFile = function (path, data, encoding, preserveBOM, callback) {
        WriteFile(callback || _dummyCallback, path, data, encoding, preserveBOM);
    };
    
    /**
     * Set permissions for a file or directory.
     *
     * @param {string} path The path of the file or directory
     * @param {number} mode The permissions for the file or directory, in numeric format (ie 0777)
     * @param {function(err)=} callback Asynchronous callback function. The callback gets one argument (err).
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
        SetPosixPermissions(callback || _dummyCallback, path, mode);
    };
    
    /**
     * Delete a file permanently.
     *
     * @param {string} path The path of the file to delete
     * @param {function(err)=} callback Asynchronous callback function. The callback gets one argument (err).
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
        DeleteFileOrDirectory(callback || _dummyCallback, path);
    };
    
    /**
     * Delete a file non-permanently (to trash).
     *
     * @param {string} path The path of the file or directory to delete
     * @param {function(err)=} callback Asynchronous callback function. The callback gets one argument (err).
     *        Possible error values:
     *          NO_ERROR
     *          ERR_UNKNOWN
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *          ERR_NOT_FILE
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function MoveFileOrDirectoryToTrash();
    appshell.fs.moveToTrash = function (path, callback) {
        MoveFileOrDirectoryToTrash(callback || _dummyCallback, path);
    };    

    /**
     * Copy src to dest, replacing the file at dest if it already exists.
     *
     * @param {string} path The path of the file to copy.
     * @param {string} data The destination to copy the file to
     * @param {function(err)} callback Asynchronous callback function. The callback gets one argument (err).
     *        Possible error values:
     *          NO_ERROR
     *          ERR_UNKNOWN
     *          ERR_INVALID_PARAMS
     *          ERR_UNSUPPORTED_ENCODING
     *          ERR_OUT_OF_SPACE
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function CopyFile();
    appshell.fs.copyFile = function (src, dest, callback) {
        CopyFile(callback || _dummyCallback, src, dest);
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
     * @param {function(err)=} callback Asynchronous callback function with one argument (the error)
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
            OpenLiveBrowser(callback || _dummyCallback, url, enableRemoteDebugging);
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
        CloseLiveBrowser(callback || _dummyCallback);
    };
 
    /**
     * Open a URL in the default OS browser window. 
     *
     * @param {function(err)} callback Asynchronous callback function with one argument (the error)
     *          NOTE: null or undefined can be used for this argument to avoid the callback
     * @param {string} url URL to open in the browser.
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function OpenURLInDefaultBrowser();
    appshell.app.openURLInDefaultBrowser = function (url, callback) {
        OpenURLInDefaultBrowser(callback || _dummyCallback, url);
    };
 
    /**
     * Get files passed to app at startup.
     *
     * @param {function(err, files)} callback Asynchronous callback function with two arguments:
     *           err - error code
     *           files - Array of file paths to open
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function GetPendingFilesToOpen();
    appshell.app.getPendingFilesToOpen = function (callback) {
        GetPendingFilesToOpen(function (err, files) {
            // "files" is a string, convert to Array
            callback(err, (err || !files) ? [] : JSON.parse(files));
        });
    };

    /**
     * Get files and folders dropped onto the application.
     *
     * @param {function(err, files)} callback Asynchronous callback function with two arguments:
     *          err - error code
     *          files - Array of file paths
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function GetDroppedFiles();
    appshell.app.getDroppedFiles = function (callback) {
        GetDroppedFiles(function (err, files) {
            // "files" is a string, convert to Array
            callback(err, (err || !files) ? [] : JSON.parse(files));
        });
    };
 
    /**
     * Get the remote debugging port used by the appshell.
     *
     * @return int. The remote debugging port used by the appshell.
     */
    native function GetRemoteDebuggingPort();
    appshell.app.getRemoteDebuggingPort = function () {
        return GetRemoteDebuggingPort();
    };
 
    /**
     * Set menu enabled/checked state.
     * @param {string} command ID of the menu item.
     * @param {bool} enabled bool to enable or disable the command
     * @param {bool} checked bool to set the 'checked' attribute of the menu item.
     * @param {?function(integer)} callback Asynchronous callback function. The callback gets one argument, error code.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     * @return None. This is an asynchronous call that is not meant to have a return
     */
    native function SetMenuItemState();
    appshell.app.setMenuItemState = function (commandid, enabled, checked, callback) {
        SetMenuItemState(callback || _dummyCallback, commandid, enabled, checked);
    };

    /**
     * Get menu enabled/checked state. For tests.
     * @param {string} command ID of the menu item.
     * @param {function(integer, bool, bool)} callback Asynchronous callback function.
     *      The callback gets three arguments, error code, enabled, checked.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     * @return None. This is an asynchronous call that is not meant to have a return
     */
    native function GetMenuItemState();
    appshell.app.getMenuItemState = function (commandid, callback) {
        GetMenuItemState(callback, commandid);
    };

    /**
     * Add a top level menu.
     * @param {string} title Menu title to display, e.g. "File"
     * @param {string} id Menu ID, e.g. "file"
     * @param {string} position Where to put the item; values are "before", "after", "first", "last", and ""
     * @param {string} relativeId The ID of the menu to which is this relative, for position "before" and "after"
     * @param {?function(integer)} callback Asynchronous callback function. The callback gets one argument, error code.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     * @return None. This is an asynchronous call that is not meant to have a return
     */
    native function AddMenu();
    appshell.app.addMenu = function (title, id, position, relativeId, callback) {
        position = position || '';
        relativeId = relativeId || '';
        AddMenu(callback || _dummyCallback, title, id, position, relativeId);
    };

    /**
     * Add a menu item.
     * @param {string} parentId ID of containing menu
     * @param {string} title Menu title to display, e.g. "Open"
     * @param {string} id Command ID, e.g. "file.open"
     * @param {string} key Shortcut, e.g. "Cmd-O"
     * @param {string} displayStr Shortcut to display in menu. If "", use key.
     * @param {string} position Where to put the item; values are "before", "after", "first", "last", 
     *                    "firstInSection", "lastInSection", and ""
     * @param {string} relativeId The ID of the menu item to which is this relative, for position "before" and "after"
     * @param {?function(integer)} callback Asynchronous callback function. The callback gets one argument, error code.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     * @return None. This is an asynchronous call that is not meant to have a return
     *
     * SPECIAL NOTE ABOUT EDIT MENU ITEMS
     * The Undo, Redo, Cut, Copy, Paste, and Select All items are handled specially. The JavaScript code
     * will get the first chance to handle the events. If they are *not* handled by JavaScript, the default
     * implementation will call those methods on the CEF instance.
     *
     * In order for this to work correctly, you MUST use the following ids for these menu item:
     *   Undo:        "edit.undo"
     *   Redo:        "edit.redo"
     *   Cut:         "edit.cut"
     *   Copy:        "edit.copy"
     *   Paste:       "edit.paste"
     *   Select All:  "edit.selectAll"
     */
    native function AddMenuItem();
    appshell.app.addMenuItem = function (parentId, title, id, key, displayStr, position, relativeId, callback) {
        key = key || '';
        position = position || '';
        relativeId = relativeId || '';
        AddMenuItem(callback || _dummyCallback, parentId, title, id, key, displayStr, position, relativeId);
    };

    /**
     * Change the title of a menu or menu item.
     * @param {string} commandid Menu/Command ID, e.g. "file" or "file.open"
     * @param {string} title Menu title to display, e.g. "File" or "Open"
     * @param {?function(integer)} callback Asynchronous callback function. The callback gets one argument, error code.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS, ERR_NOT_FOUND, ERR_UNKNOWN
     * @return None. This is an asynchronous call that is not meant to have a return
     */
    native function SetMenuTitle();
    appshell.app.setMenuTitle = function (commandid, title, callback) {
        SetMenuTitle(callback || _dummyCallback, commandid, title);
    };
 
    /**
     * Get menu title. For tests.
     * @param {string} commandid ID of the menu item.
     * @param {function(integer, string)} callback Asynchronous callback function.
     *      The callback gets two arguments, error code, title.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     * @return None. This is an asynchronous call that is not meant to have a return
     */
    native function GetMenuTitle();
    appshell.app.getMenuTitle = function (commandid, callback) {
        GetMenuTitle(callback, commandid);
    };

    /**
     * Set menu item shortuct. 
     * @param {string} commandId ID of the menu item.
     * @param {string} shortcut Shortcut string, like "Cmd-U".
     * @param {string} displayStr String to display in menu. If "", use shortcut.
     * @param {?function (err)} callback Asynchronous callback function. The callback gets an error code.
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function SetMenuItemShortcut();
    appshell.app.setMenuItemShortcut = function (commandId, shortcut, displayStr, callback) {
        SetMenuItemShortcut(callback || _dummyCallback, commandId, shortcut, displayStr);
    };
 
    /**
     * Remove menu associated with commandId.
     * @param {string} commandid ID of the menu item.
     * @param {function(err)=} callback Asynchronous callback function. The callback gets an error code.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *                 
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function RemoveMenu();
    appshell.app.removeMenu = function (commandId, callback) {
        RemoveMenu(callback || _dummyCallback, commandId);
    };

    /**
     * Remove menuitem associated with commandId.
     * @param {string} commandid ID of the menu item.
     * @param {function(err)=} callback Asynchronous callback function. The callback gets an error code.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *                 
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function RemoveMenuItem();
    appshell.app.removeMenuItem = function (commandId, callback) {
        RemoveMenuItem(callback || _dummyCallback, commandId);
    };
 
    /**
     * Get the position of the menu or menu item associated with commandId.
     * @param {string} command ID of the menu or menu item.
     * @param {function(err, parentId, index)} callback Asynchronous callback function. The callback gets an error code,
     *     the ID of the immediate parent and the index of the menu or menu item. If the command is a top level menu,
     *     it does not have a parent and parentId will be NULL. The returned index will be -1 if the command is not found.
     *     In those not-found cases, the error value will also be ERR_NOT_FOUND.
     *        Possible error values:
     *          NO_ERROR
     *          ERR_INVALID_PARAMS
     *          ERR_NOT_FOUND
     *                 
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function GetMenuPosition();
    appshell.app.getMenuPosition = function (commandId, callback) {
        GetMenuPosition(callback, commandId);
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
     * On the Mac, it's /Users/<user>/Library/Application Support[/GROUP_NAME]/APP_NAME
     * On Windows, it's C:\Users\<user>\AppData\Roaming[\GROUP_NAME]\APP_NAME
     *
     * @return {string} Full path of the application support directory
     */
    native function GetApplicationSupportDirectory();
    appshell.app.getApplicationSupportDirectory = function () {
        return GetApplicationSupportDirectory();
    };
  
    /**
     * Returns the full path of the user's documents directory.
     * On the Mac, it's /Users/<user>/Documents
     * On Windows, it's C:\Users\<user>\Documents (Windows Vista, 7, 8),
     *                  C:\Documents and Settings\<user>\My Documents (Windows XP)
     *
     * @return {string} Full path of the application support directory
     */
    native function GetUserDocumentsDirectory();
    appshell.app.getUserDocumentsDirectory = function () {
        return GetUserDocumentsDirectory();
    };


    /**
     * Open the specified folder in an OS file window.
     *
     * @param {string} path Path of the folder to show.
     * @param {function(err)=} callback Asyncrhonous callback function with one argument (the error)
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function ShowOSFolder();
    appshell.app.showOSFolder = function (path, callback) {
        ShowOSFolder(callback || _dummyCallback, path);
    };
 
    /**
     * Open the extensions folder in an OS file window.
     *
     * @param {string} appURL Not used
     * @param {function(err)=} callback Asynchronous callback function with one argument (the error)
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    appshell.app.showExtensionsFolder = function (appURL, callback) {
        appshell.app.showOSFolder(GetApplicationSupportDirectory() + "/extensions", callback);
    };

    /**
     * Drag the main window
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function DragWindow();
    appshell.app.dragWindow = function () {
        DragWindow();
    };

    /**
     * Get the browser zoom level
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function GetZoomLevel();
    appshell.app.getZoomLevel = function (callback) {
        GetZoomLevel(callback || _dummyCallback);
    };

    /**
     * Set the browser zoom level
     *
     * @param {number} 
     *
     * @return None. This is an asynchronous call that sends all return information to the callback.
     */
    native function SetZoomLevel();
    appshell.app.setZoomLevel = function (zoomLevel, callback) {
        SetZoomLevel(callback || _dummyCallback, zoomLevel);
    };

    /**
     * Install command line scripts to make Brackets launchable from command line.
     * Right now usage is restricted to MAC only.
     *
     * @param {number}
     *
     * @return none.
     */
     native function InstallCommandLineTools();
     appshell.app.installCommandLine = function (callback) {
        InstallCommandLineTools(callback);
     };

     /**
      * Get hash of the machine based on various parameters like
      * network interfaces, CPU ID, volume serial number e.t.c
      *
      * @param {none}
      *
      * @return None. This is an asynchronous call that sends all return information to the callback.
      */
     native function GetMachineHash();
     appshell.app.getMachineHash = function (callback) {
         GetMachineHash(callback || _dummyCallback);
     };

 
    // Alias the appshell object to brackets. This is temporary and should be removed.
    brackets = appshell;
})();
