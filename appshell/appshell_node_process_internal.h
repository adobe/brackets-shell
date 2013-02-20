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

#include <string>

// This file declares "internal" functions that exist on both plantforms for
// managing the state of the node process. Only code internal to the
// management of node should call these functions. The reason we have an include
// file at all is to avoid duplication of forward declarations in multiple
// platform-specific files.

// Cross-platform function to process incoming data. Buffers data, parses
// the data into messages, and responds appropriately (possibly by calling
// platform-specific functions below).
void processIncomingData(const std::string &data);

// Platform-specific functions that must be be present on all platforms.
// All of these functions below must be implemented in
// a thread-safe manner if calls to the *public* API (defined in
// appshell_node_process.h) happen from a different thread than that which
// calls processIncomingData.

// Sends data to the Node process, if there is one. Drops the data if there
// isn't a Node process.
void sendData(const std::string &data);

// Sets the state of the current Node process.
void setNodeState(int state);
