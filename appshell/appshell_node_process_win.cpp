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

#include <windows.h>
#include <Shlwapi.h>
#include <MMSystem.h>

#include "appshell_node_process.h"
#include "appshell_node_process_internal.h"

#include <strsafe.h> // must be included after STL headers

#ifndef OS_WIN
#define OS_WIN 1
#endif
#include "config.h"

#define BRACKETS_NODE_BUFFER_SIZE 4096

static HANDLE g_hChildStd_IN_Rd = NULL;
static HANDLE g_hChildStd_IN_Wr = NULL;
static HANDLE g_hChildStd_OUT_Rd = NULL;
static HANDLE g_hChildStd_OUT_Wr = NULL;

static volatile HANDLE hNodeThread = NULL;
static volatile HANDLE hNodeReadThread = NULL;
static volatile HANDLE hNodeMutex = NULL;

// Threads should hold hNodeMutex before using these variables
static volatile int nodeState = BRACKETS_NODE_NOT_YET_STARTED;
static volatile DWORD nodeStartTime = 0;
static PROCESS_INFORMATION piProcInfo;

// Forward declarations
DWORD WINAPI NodeThread(LPVOID);
void restartNode(bool);

// Creates the thread that starts Node and then monitors the state
// of the node process.
void startNodeProcess() {
	// IMPORTANT THREAD SAFETY NOTE: Currently, this function is called once
	// during startup from the main thread, and then all other times from the
	// NodeThread thread. Since the first call to the function actually
	// *starts* the NodeThread thread, there's currently no way this function
	// could get called simultaneously by two processes.
	//
	// However, if we ever implement a way for the main process to re-call
	// this function, we will need to wrap it in a separate mutex to ensure
	// that we don't start two node processes.

	if (hNodeMutex == NULL) {
		hNodeMutex = CreateMutex(NULL, false, NULL);
	}

	if (hNodeMutex == NULL) { // If mutex is STILL null, there's an unrecoverable error
		nodeState = BRACKETS_NODE_FAILED;
	} else {
		if (getNodeState() == BRACKETS_NODE_NOT_YET_STARTED) {
			hNodeThread = CreateThread(NULL, 0, NodeThread, NULL, 0, NULL);
		}
	}
}

// Thread function for the thread that reads from the Node pipe
// Reads on anonymous pipes are always blocking (OVERLAPPED reads
// are not possible) So, we need to do this in a separate thread
//
// TODO: This code first reads to a character buffer, and then
// copies it to a std::string. The code could be optimized to avoid
// this double-copy
DWORD WINAPI NodeReadThread(LPVOID lpParam) {
	DWORD dwRead;
	CHAR chBuf[BRACKETS_NODE_BUFFER_SIZE];
	std::string strBuf("");
	BOOL bSuccess = FALSE;
	for (;;) {	
		bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BRACKETS_NODE_BUFFER_SIZE, &dwRead, NULL);
		if( ! bSuccess || dwRead == 0 ) {
			break;
		} else {
			strBuf.assign(chBuf, dwRead);
			processIncomingData(strBuf);
		}
	} 
	return 0;
}

// Thread function for the thread that starts the node process
// and monitors that process's state
DWORD WINAPI NodeThread(LPVOID lpParam) {
	// We hold the mutex during startup, and then release it right before
	// we start our read loop
	if (hNodeMutex != NULL) {
		DWORD dwWaitResult = WaitForSingleObject(hNodeMutex, INFINITE);
		if (dwWaitResult == WAIT_OBJECT_0) { // got the mutex
			nodeStartTime = timeGetTime();

			HMODULE module = GetModuleHandle(NULL);
			TCHAR executablePath[MAX_UNC_PATH];
			TCHAR scriptPath[MAX_UNC_PATH];
			TCHAR commandLine[BRACKETS_NODE_BUFFER_SIZE];

			DWORD dwFLen = GetModuleFileName(module, executablePath, MAX_UNC_PATH);
			if (dwFLen == 0) {
				fprintf(stderr, "[Node] Could not get module path");
				ReleaseMutex(hNodeMutex);
				restartNode(false);
				return 0;
			} else if (dwFLen == MAX_UNC_PATH) {
				fprintf(stderr, "[Node] Path to module exceede max path length");
				ReleaseMutex(hNodeMutex);
				restartNode(false);
				return 0;
			}
			PathRemoveFileSpec(executablePath);
			StringCchCopy(scriptPath, MAX_UNC_PATH, executablePath);
			PathAppend(executablePath, TEXT(NODE_EXECUTABLE_PATH));
			PathAppend(scriptPath, TEXT(NODE_CORE_PATH));

			StringCchCopy(commandLine, BRACKETS_NODE_BUFFER_SIZE, TEXT("\""));
			StringCchCat(commandLine, BRACKETS_NODE_BUFFER_SIZE, executablePath);
			StringCchCat(commandLine, BRACKETS_NODE_BUFFER_SIZE, TEXT("\" \""));
			StringCchCat(commandLine, BRACKETS_NODE_BUFFER_SIZE, scriptPath);
			StringCchCat(commandLine, BRACKETS_NODE_BUFFER_SIZE, TEXT("\""));

			SECURITY_ATTRIBUTES saAttr; 

			// Set the bInheritHandle flag so pipe handles are inherited. 

			saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
			saAttr.bInheritHandle = TRUE; 
			saAttr.lpSecurityDescriptor = NULL; 

			// Create a pipe for the child process's STDOUT. 
			if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
				ReleaseMutex(hNodeMutex);
				restartNode(false);
				return 0;
			}

			// Ensure the read handle to the pipe for STDOUT is not inherited.
			if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
				ReleaseMutex(hNodeMutex);
				restartNode(false);
				return 0;
			}

			// Create a pipe for the child process's STDIN. 
			if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
				ReleaseMutex(hNodeMutex);
				restartNode(false);
				return 0;
			}

			// Ensure the write handle to the pipe for STDIN is not inherited. 
			if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
				ReleaseMutex(hNodeMutex);
				restartNode(false);
				return 0; 
			}

			// Create the child process.
 
			STARTUPINFO siStartInfo;
			BOOL bSuccess = FALSE; 

			// Set up members of the PROCESS_INFORMATION structure. 
			ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

			// Set up members of the STARTUPINFO structure. 
			// This structure specifies the STDIN and STDOUT handles for redirection.

			ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
			siStartInfo.cb = sizeof(STARTUPINFO); 
			siStartInfo.hStdError = g_hChildStd_OUT_Wr;
			siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
			siStartInfo.hStdInput = g_hChildStd_IN_Rd;
			siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

			// Create the child process. 

			bSuccess = CreateProcess(NULL, 
				commandLine,       // command line 
				NULL,              // process security attributes 
				NULL,              // primary thread security attributes 
				TRUE,              // handles are inherited 
				CREATE_NO_WINDOW,  // creation flags (change to 0 to see a window for the launched process) 
				NULL,              // use parent's environment 
				NULL,              // use parent's current directory 
				&siStartInfo,      // STARTUPINFO pointer 
				&piProcInfo);      // receives PROCESS_INFORMATION 

			nodeState = BRACKETS_NODE_PORT_NOT_YET_SET;

			// Done launching the process, so release the mutex and start reading
			ReleaseMutex(hNodeMutex);

			if (!bSuccess) {
				restartNode(false);
				return 0;
			}
			else {
				// Start reading from the pipe
				hNodeReadThread = CreateThread(NULL, 0, NodeReadThread, NULL, 0, NULL);

				// Loop to check if process is still running
				BOOL bSuccess = FALSE;
				DWORD exitCode = 0;
				for (;;) 
				{	
					bSuccess = GetExitCodeProcess(piProcInfo.hProcess, &exitCode);
					if (bSuccess && exitCode != STILL_ACTIVE) {
						TerminateThread(hNodeReadThread, 0);
						break; // process exited, possibly restart
					} else {
						Sleep(1000);
					}
				} 
				
				// If we broke out of this loop, the node process has terminated
				// So, we should try to restart.
				restartNode(false);
			}
		}
	}

	return 0;
}

// Determines whether the current node process has run long
// enough that a restart is warranted, and initiates the startup
// if so. Can also optionally terminate the running process.
void restartNode(bool terminateCurrentProcess) {
	bool shouldRestart = false;

	if (hNodeMutex != NULL) {
		DWORD dwWaitResult;
		dwWaitResult = WaitForSingleObject(hNodeMutex, INFINITE);
		if (dwWaitResult == WAIT_OBJECT_0) { // got the mutex
			if (terminateCurrentProcess && piProcInfo.hProcess != NULL) {
				TerminateProcess(piProcInfo.hProcess, 0);
			}

			// Close any handles to the old process that are still open
			if (piProcInfo.hProcess != NULL) {
				CloseHandle(piProcInfo.hProcess);
				piProcInfo.hProcess = NULL;
			}
			if (piProcInfo.hThread != NULL) {
				CloseHandle(piProcInfo.hThread);
				piProcInfo.hThread = NULL;
			}

			// we hold mutex, so okay to set this directly instead of
			// calling setNodeState
			nodeState = BRACKETS_NODE_NOT_YET_STARTED;
			
			// Then, check if we were running long enough to restart
			DWORD now = timeGetTime();
			shouldRestart = (now - nodeStartTime > (BRACKETS_NODE_AUTO_RESTART_TIMEOUT * 1000));
			
			// Need to release the mutex before possibly restarting,
			// since startNodePorcess wants the mutex
			ReleaseMutex(hNodeMutex);

			if (shouldRestart) {
				// Ran at least 5 seconds last time, so restart
				startNodeProcess();
			} else {
				// Didn't run long enough
				setNodeState(BRACKETS_NODE_FAILED);
			}			

		}
	}
	
}

// Sends data to the node process. If the write fails completely,
// calls restartNode.
void sendData(const std::string &data) {
	if (hNodeMutex != NULL) {
		DWORD dwWaitResult;
		DWORD dwWritten;
		BOOL bSuccess = FALSE;
		dwWaitResult = WaitForSingleObject(hNodeMutex, INFINITE);
		if (dwWaitResult == WAIT_OBJECT_0) { // got the mutex
			bSuccess = WriteFile(g_hChildStd_IN_Wr, data.c_str(), data.length(), &dwWritten, NULL);
			ReleaseMutex(hNodeMutex);
			if (!bSuccess) {
				// Failed to write, there's something wrong with this process.
				// Restart it.
				restartNode(true);
			} else if (dwWritten < data.length()) {
				// Send succeeded, but didn't write all the data, so try the rest.
				sendData(data.substr(dwWritten));
			}
		} else {
			fprintf(stderr, "[Node] Unable to get mutex in order to send data");
		}
	}
	// If the mutex didn't exist or we were unable to get it, there's a
	// problem with the node process, so we simply drop the data.
}

// Thread-safe way to access nodeState variable 
int getNodeState() {
	// The very first thing we do when starting Node is create a mutex.
	// So, if that mutex doesn't exist, we haven't started.
	int result = BRACKETS_NODE_NOT_YET_STARTED;
	
	if (hNodeMutex != NULL) {
		// if there was a mutex and we don't get it, then something is 
		// seriously wrong, so set a default response of BRACKETS_NODE_FAILED 
		result = BRACKETS_NODE_FAILED;

		DWORD dwWaitResult;
		dwWaitResult = WaitForSingleObject(hNodeMutex, INFINITE);
		if (dwWaitResult == WAIT_OBJECT_0) { // got the mutex
			result = nodeState;
			ReleaseMutex(hNodeMutex);
		}
	}

	return result;
}

// Thread-safe way to set nodeState variable
void setNodeState(int newState) {
	if (hNodeMutex != NULL) {
		DWORD dwWaitResult;
		dwWaitResult = WaitForSingleObject(hNodeMutex, INFINITE);
		if (dwWaitResult == WAIT_OBJECT_0) { // got the mutex
			nodeState = newState;
			ReleaseMutex(hNodeMutex);
		}
		// If we didn't get the mutex, we don't have any way to signal the state
		// here. But something is very wrong, so something internally should eventually
		// set the state to BRACKETS_NODE_FAILED. (Or, a call to getNodeState also won't
		// be able to get the mutex, and so it will return BRACKETS_NODE_FAILED.)
	}
	// If the mutex doesn't exist yet, then no one has ever started the node process.
	// So, it's not appropriate for something to be setting the state. So, we do nothing
	// (This should not happen, as this function is only called by way of a running node process)
}
