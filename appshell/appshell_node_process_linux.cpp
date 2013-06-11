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

#include "appshell_node_process.h"
#include "appshell_node_process_internal.h"

#ifndef OS_LINUX
#define OS_LINUX 1
#endif
#include "config.h"

#define BRACKETS_NODE_BUFFER_SIZE 4096

// Forward declarations
//DWORD WINAPI NodeThread(LPVOID);
void restartNode(bool);

// Creates the thread that starts Node and then monitors the state
// of the node process.
void startNodeProcess() {
}

// Thread function for the thread that reads from the Node pipe
// Reads on anonymous pipes are always blocking (OVERLAPPED reads
// are not possible) So, we need to do this in a separate thread
//
// TODO: This code first reads to a character buffer, and then
// copies it to a std::string. The code could be optimized to avoid
// this double-copy
//DWORD WINAPI NodeReadThread(LPVOID lpParam) {
//}

// Thread function for the thread that starts the node process
// and monitors that process's state
//DWORD WINAPI NodeThread(LPVOID lpParam) {
//}

// Determines whether the current node process has run long
// enough that a restart is warranted, and initiates the startup
// if so. Can also optionally terminate the running process.
void restartNode(bool terminateCurrentProcess) {
}

// Sends data to the node process. If the write fails completely,
// calls restartNode.
void sendData(const std::string &data) {
}

// Thread-safe way to access nodeState variable 
int getNodeState() {
}

// Thread-safe way to set nodeState variable
void setNodeState(int newState) {
}
