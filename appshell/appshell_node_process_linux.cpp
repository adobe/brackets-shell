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

// write & read references to subprocess
FILE* streamTo;
FILE* streamFrom;

// Threads should hold mutex before using these
static int nodeState = BRACKETS_NODE_NOT_YET_STARTED;
static int nodeStartTime = 0;

// Forward declarations
void* nodeThread(void*);
void* nodeReadThread(void*);
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
    if (pthread_mutex_lock(&mutex)) {
        fprintf(stderr, 
                "failed to acquire mutex for Node subprocess start: %s\n",
                strerror(errno));
        return NULL;
    }        
    
    // TODO nodeStartTime = get time();
    
    char executablePath[MAX_PATH];
    char bracketsDirPath[MAX_PATH];
    char nodeExecutablePath[MAX_PATH];
    char nodecorePath[MAX_PATH];

    // get path to Brackets
    if (readlink("/proc/self/exe", executablePath, MAX_PATH) == -1) {
        fprintf(stderr, "cannot find Brackets path: %s\n", strerror(errno));
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    // strip off trailing executable name
    char* lastIndexOf = strrchr(executablePath, '/');
    memcpy(bracketsDirPath, executablePath, lastIndexOf - executablePath + 1);
    
    // create node exec and node-core paths
    strcpy(nodeExecutablePath, bracketsDirPath);
    strcat(nodeExecutablePath, NODE_EXECUTABLE_PATH);
    strcpy(nodecorePath, bracketsDirPath);
    strcat(nodecorePath, NODE_CORE_PATH);
    
    // create pipes for node process stdin/stdout
    int toNode[2];
    int fromNode[2];
    
    pipe(toNode);
    pipe(fromNode);
    
    // create the Node process
    pid_t child_pid = fork();
    if (child_pid == 0) { // child (node) process
        // close our copy of write end and
        // connect read end to stdin
        close(toNode[1]);
        dup2(toNode[0], STDIN_FILENO);
        
        // close our copy of read end and
        // connect write end to stdout
        close(fromNode[0]);
        dup2(fromNode[1], STDOUT_FILENO);
        
        // run node executable
        char* arg_list[] = { nodeExecutablePath, nodecorePath, NULL};
        execvp(arg_list[0], arg_list);
        
        fprintf(stderr, "the Node process failed to start: %s\n", strerror(errno));
        abort();
    }
    else { // parent
        
        // close our reference of toNode's read end
        // and fromNode's write end
        close(toNode[0]);
        close(fromNode[1]);
        
        // convert subprocess write & read to FILE objects
        streamTo = fdopen(toNode[1], "w");
        streamFrom = fdopen(fromNode[0], "r");

        nodeState = BRACKETS_NODE_PORT_NOT_YET_SET;
    
        // done launching process so release mutex
        if (pthread_mutex_unlock(&mutex)) {
            fprintf(stderr, 
                "failed to release mutex for Node subprocess startup: %s\n",
                strerror(errno));
        }
        
        // start pipe read thread
        pthread_t readthread_id;
        if (pthread_create(&readthread_id, NULL, &nodeReadThread, NULL) != 0)
            nodeState = BRACKETS_NODE_FAILED;
            // ugly - need to think more about what to do if read thread fails

    }
    
    return NULL;
}


// Thread function for the thread that reads from the Node pipe
// Reads on anonymous pipes are always blocking (OVERLAPPED reads
// are not possible) So, we need to do this in a separate thread

// TODO: This code first reads to a character buffer, and then
// copies it to a std::string. The code could be optimized to avoid
// this double-copy
void* nodeReadThread(void* unused) {
    
    char charBuf[BRACKETS_NODE_BUFFER_SIZE];
    std::string strBuf("");
    while (fgets(charBuf, BRACKETS_NODE_BUFFER_SIZE, streamFrom) != NULL) {
        strBuf.assign(charBuf);
        processIncomingData(strBuf);
    }
}


// Determines whether the current node process has run long
// enough that a restart is warranted, and initiates the startup
// if so. Can also optionally terminate the running process.
void restartNode(bool terminateCurrentProcess) {
    
}

// Sends data to the node process. If the write fails completely,
// calls restartNode.
void sendData(const std::string &data) {
    
    if (pthread_mutex_lock(&mutex)) {
        fprintf(stderr, 
            "failed to acquire mutex for write to Node subprocess: %s\n",
            strerror(errno));
        return;
    }

    // write to pipe
    fprintf(streamTo, "%s", data.c_str());
    
    if (pthread_mutex_unlock(&mutex)) {
            fprintf(stderr, 
            "failed to release mutex for write to Node subprocess: %s\n",
            strerror(errno));
    }
}

// Returns nodeState variable 
int getNodeState() {
    
    return nodeState;
}

// Thread-safe way to set nodeState variable
void setNodeState(int newState) {

    if (pthread_mutex_lock(&mutex)) {
            fprintf(stderr, 
            "failed to acquire mutex for setting Node state: %s\n",
            strerror(errno));
            return;
    }
    nodeState = newState;
    if (pthread_mutex_unlock(&mutex)) {
            fprintf(stderr, 
            "failed to release mutex for Node set state: %s\n",
            strerror(errno));
    }
}
