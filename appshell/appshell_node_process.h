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

#pragma once

// Node error codes. These MUST be in sync with the error
// codes in appshell_extensions.js
static const int BRACKETS_NODE_NOT_YET_STARTED  = -1;
static const int BRACKETS_NODE_PORT_NOT_YET_SET = -2;
static const int BRACKETS_NODE_FAILED           = -3;

// For auto restart, if node ran for less than 5 seconds, do NOT do a restart
static const int BRACKETS_NODE_AUTO_RESTART_TIMEOUT = 5; // seconds

// Public interface for interacting with node process. All of these functions below
// must be implemented in a thread-safe manner if calls to the *public* API happen
// from a different thread than that which processes data coming in from the Node
// process.

// Start the Node process. If a process is already running, this is a no-op.
void startNodeProcess();

// Gets the state of the current Node process. If the value is < 0, it indicates
// an error as defined above. If the value is >= 0, it indicates the socket port
// that the Node server is listening on.
int getNodeState();

