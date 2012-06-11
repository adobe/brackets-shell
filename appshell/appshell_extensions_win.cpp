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

#include "appshell_extensions.h"

#include <algorithm>
#include <CommDlg.h>
#include <ShlObj.h>

// Forward declarations for functions at the bottom of this file
void FixFilename(ExtensionString& filename);
int ConvertErrnoCode(int errorCode, bool isReading = true);
int ConvertWinErrorCode(int errorCode, bool isReading = true);

static int CALLBACK SetInitialPathCallback(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (BFFM_INITIALIZED == uMsg && NULL != lpData)
    {
        SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
    }

    return 0;
}

int32 ShowOpenDialog(bool allowMulitpleSelection,
                     bool chooseDirectory,
                     ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString fileTypes,
                     CefRefPtr<CefListValue>& selectedFiles)
{
    wchar_t szFile[MAX_PATH];
    szFile[0] = 0;

    FixFilename(initialDirectory);

    // TODO (issue #64) - This method should be using IFileDialog instead of the
    /* outdated SHGetPathFromIDList and GetOpenFileName.
       
    Useful function to parse fileTypesStr:
    template<class T>
    int inline findAndReplaceString(T& source, const T& find, const T& replace)
    {
    int num=0;
    int fLen = find.size();
    int rLen = replace.size();
    for (int pos=0; (pos=source.find(find, pos))!=T::npos; pos+=rLen)
    {
    num++;
    source.replace(pos, fLen, replace);
    }
    return num;
    }
    */

    if (chooseDirectory) {
        BROWSEINFO bi = {0};
        bi.hwndOwner = GetActiveWindow();
        bi.lpszTitle = title.c_str();
        bi.ulFlags = BIF_NEWDIALOGSTYLE;
        bi.lpfn = SetInitialPathCallback;
        bi.lParam = (LPARAM)initialDirectory.c_str();

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl != 0) {
            if (SHGetPathFromIDList(pidl, szFile)) {
                // Add directory path to the result
                selectedFiles->SetString(0, szFile);
            }
            IMalloc* pMalloc = NULL;
            SHGetMalloc(&pMalloc);
            if (pMalloc) {
                pMalloc->Free(pidl);
                pMalloc->Release();
            }
        }
    } else {
        OPENFILENAME ofn;

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.hwndOwner = GetActiveWindow();
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;

        allowMulitpleSelection = false;  // TODO: Raymond, please implement.

        // TODO (issue #65) - Use passed in file types. Note, when fileTypesStr is null, all files should be shown
        /* findAndReplaceString( fileTypesStr, std::string(" "), std::string(";*."));
        LPCWSTR allFilesFilter = L"All Files\0*.*\0\0";*/

        ofn.lpstrFilter = L"All Files\0*.*\0Web Files\0*.js;*.css;*.htm;*.html\0\0";
           
        ofn.lpstrInitialDir = initialDirectory.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
        if (allowMulitpleSelection)
            ofn.Flags |= OFN_ALLOWMULTISELECT;

        if (GetOpenFileName(&ofn)) {
            if (allowMulitpleSelection) {
                // Multiple selection encodes the files differently
/*
                // If multiple files are selected, the first null terminator
                // signals end of directory that the files are all in
                std::wstring dir(szFile);

                // Check for two null terminators, which signal that only one file
                // was selected
                if (szFile[dir.length() + 1] == '\0') {
                    // Escape the single file path and add it to the JSON array
                    std::wstring escaped;
                    EscapeJSONString(dir, escaped);
                    results += L"\"" + escaped + L"\"";
                } else {
                    // Multiple files are selected
                    wchar_t fullPath[MAX_PATH];
                    bool firstFile = true;
                    for (int i = dir.length() + 1;;) {
                        // Get the next file name
                        std::wstring file(&szFile[i]);

                        // Two adjacent null characters signal the end of the files
                        if (file.length() == 0)
                            break;

                        // The filename is relative to the directory that was specified as
                        // the first string
                        if (PathCombine(fullPath, dir.c_str(), file.c_str()) != NULL)
                        {
                            // Append a comma separator if it is not the first file in the list
                            if (firstFile)
                                firstFile = false;
                            else
                                results += L",";

                            // Escape the path and add it to the list
                            std::wstring escaped;
                            EscapeJSONString(std::wstring(fullPath), escaped);
                            results += L"\"" + escaped + L"\"";
                        }

                        // Go to the start of the next file name
                        i += file.length() + 1;
                    }
                }
*/
            } else {
                // If multiple files are not allowed, add the single file
                selectedFiles->SetString(0, szFile);
            }
        }
    }

    return NO_ERROR;
}

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents)
{
    FixFilename(path);

    path += L"\\*";

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(path.c_str(), &ffd);

    std::vector<ExtensionString> resultFiles;
    std::vector<ExtensionString> resultDirs;

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Ignore '.' and '..'
            if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L".."))
                continue;

            // Collect file and directory names separately
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                resultDirs.push_back(ExtensionString(ffd.cFileName));
            } else {
                resultFiles.push_back(ExtensionString(ffd.cFileName));
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);

        FindClose(hFind);
    } 
    else {
        return ConvertWinErrorCode(GetLastError());
    }

    // On Windows, list directories first, then files
    size_t i, total = 0;
    for (i = 0; i < resultDirs.size(); i++)
        directoryContents->SetString(total++, resultDirs[i]);
    for (i = 0; i < resultFiles.size(); i++)
        directoryContents->SetString(total++, resultFiles[i]);

    return NO_ERROR;
}

int32 GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir)
{
    FixFilename(filename);

    DWORD dwAttr = GetFileAttributes(filename.c_str());

    if (dwAttr == INVALID_FILE_ATTRIBUTES) {
        return ConvertWinErrorCode(GetLastError()); 
    }

    isDir = ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);

    struct _stat buffer;
    if(_wstat(filename.c_str(), &buffer) == -1) {
        return ConvertErrnoCode(errno); 
    }

    modtime = buffer.st_mtime;

    return NO_ERROR;
}

int32 ReadFile(ExtensionString filename, ExtensionString encoding, std::string& contents)
{
    FixFilename(filename);

    if (encoding != L"utf8")
        return ERR_UNSUPPORTED_ENCODING;

    DWORD dwAttr;
    dwAttr = GetFileAttributes(filename.c_str());
    if (INVALID_FILE_ATTRIBUTES == dwAttr)
        return ConvertWinErrorCode(GetLastError());

    if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
        return ERR_CANT_READ;

    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ,
        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    int32 error = NO_ERROR;

    if (INVALID_HANDLE_VALUE == hFile)
        return ConvertWinErrorCode(GetLastError()); 

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    DWORD dwBytesRead;
    char* buffer = (char*)malloc(dwFileSize);
    if (buffer && ReadFile(hFile, buffer, dwFileSize, &dwBytesRead, NULL)) {
        contents = std::string(buffer, dwFileSize);
    }
    else {
        if (!buffer)
            error = ERR_UNKNOWN;
        else
            error = ConvertWinErrorCode(GetLastError());
    }
    CloseHandle(hFile);
    if (buffer)
        free(buffer);

    return error; 
}

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding)
{
    FixFilename(filename);

    if (encoding != L"utf8")
        return ERR_UNSUPPORTED_ENCODING;

    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD dwBytesWritten;
    int error = NO_ERROR;

    if (INVALID_HANDLE_VALUE == hFile)
        return ConvertWinErrorCode(GetLastError(), false); 

    // TODO (issue 67) -  Should write to temp file and handle encoding
    if (!WriteFile(hFile, contents.c_str(), contents.length(), &dwBytesWritten, NULL)) {
        error = ConvertWinErrorCode(GetLastError(), false);
    }

    CloseHandle(hFile);
    return error;
}

int32 SetPosixPermissions(ExtensionString filename, int32 mode)
{
    // TODO: Raymond, please implement
    return NO_ERROR;
}

int32 DeleteFileOrDirectory(ExtensionString filename)
{
    // TODO: Raymond, please implement
    return NO_ERROR;
}

void FixFilename(ExtensionString& filename)
{
    // Convert '/' to '\'
    replace(filename.begin(), filename.end(), '/', '\\');
}

// Maps errors from errno.h to the brackets error codes
// found in brackets_extensions.js
int ConvertErrnoCode(int errorCode, bool isReading)
{
    switch (errorCode) {
    case NO_ERROR:
        return NO_ERROR;
    case EINVAL:
        return ERR_INVALID_PARAMS;
    case ENOENT:
        return ERR_NOT_FOUND;
    default:
        return ERR_UNKNOWN;
    }
}

// Maps errors from  WinError.h to the brackets error codes
// found in brackets_extensions.js
int ConvertWinErrorCode(int errorCode, bool isReading)
{
    switch (errorCode) {
    case NO_ERROR:
        return NO_ERROR;
    case ERROR_PATH_NOT_FOUND:
    case ERROR_FILE_NOT_FOUND:
        return ERR_NOT_FOUND;
    case ERROR_ACCESS_DENIED:
        return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
    case ERROR_WRITE_PROTECT:
        return ERR_CANT_WRITE;
    case ERROR_HANDLE_DISK_FULL:
        return ERR_OUT_OF_SPACE;
    default:
        return ERR_UNKNOWN;
    }
}

