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

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>


#ifndef OS_LINUX
#define OS_LINUX 1
#endif
#include "config.h"

#define BRACKETS_NODE_BUFFER_SIZE 4096

#define MAX_PATH 128

// init mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



// Threads should hold mutex before using these
static int nodeState = BRACKETS_NODE_NOT_YET_STARTED;
static int nodeStartTime = 0;

// Forward declarations
void* nodeThread(void*);
void restartNode(bool);

// Creates the thread that starts Node and then monitors the state
// of the node process.
void startNodeProcess() {
    
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, &nodeThread, NULL) != 0)
        nodeState = BRACKETS_NODE_FAILED;
}


// Thread function for the thread that starts the node process
// and monitors that process's state
void* nodeThread(void* unused) {
    
    // get mutex
    pthread_mutex_lock(&mutex);
    
    // TODO nodeStartTime = get time();
    
    char executablePath[MAX_PATH];
    char scriptPath[MAX_PATH];
    char commandLine[BRACKETS_NODE_BUFFER_SIZE];

    // get path to Brackets
    if (readlink("/proc/self/exe", executablePath, MAX_PATH) == -1) {
        fprintf(stderr, "cannot find Brackets path: %s\n", strerror(errno));
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    // strip off 'out/Release/Brackets' for path to brackets-shell
    // and append node executable path
    size_t pathLength = strlen(executablePath);
    memcpy(commandLine, executablePath, pathLength - 20);
    strcat(commandLine, NODE_EXECUTABLE_PATH);

    
    // create pipes for node process stdin/stdout
    int toNode[2];
    int fromNode[2]; // not yet implemented
    
    pipe(toNode);
    
    // create the Node process
    pid_t child_pid = fork();
    if (child_pid == 0) { // child (node) process
        // close our copy of write end
        close(toNode[1]);
        // connect read end to stdin
        dup2(toNode[0], STDIN_FILENO);
        
        // run node executable
        char* arg_list[] = { commandLine, NULL};
        execvp(arg_list[0], arg_list);
        
        fprintf(stderr, "the Node process failed to start: %s\n", strerror(errno));
        abort();
    }
    else { // parent
        
        FILE* stream;
        // close our copy of the pipe's read end
        close(toNode[0]);
        // convert write fd to a FILE object
        stream = fdopen(toNode[1], "w");
        fprintf(stream, "console.log('Hello World');\n");
        fflush(stream);

    }
    
    
    nodeState = BRACKETS_NODE_PORT_NOT_YET_SET;
    
    // done launching process so release mutex
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

//

// Thread function for the thread that reads from the Node pipe
// Reads on anonymous pipes are always blocking (OVERLAPPED reads
// are not possible) So, we need to do this in a separate thread


// TODO: This code first reads to a character buffer, and then
// copies it to a std::string. The code could be optimized to avoid
// this double-copy
//DWORD WINAPI NodeReadThread(LPVOID lpParam) {
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
