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

#include <sstream>
#include <vector>
#include <cstdio>

static std::string buffer("");
static int commandCount = 0;

// Processes a single command from the node process. May call
// platform-specific functions in order to do this. Any platform-specific
// functions must be declared in a file used on all platforms and then
// implemented on all platforms. Because this is not the main communication
// channel with the client, the protocol is left very simple. Individual
// messages are separated by two newlines. Arguments in a message are
// separated by "|" characters. The first argument is the command name, and
// the remaining arguments are dependent on the command.
//
// The number of total commands is expected to be very small. Right now,
// the only commands are "ping" (a keep-alive command) and "port", which
// records the port number that the node websocket server is currently using.
void processCommand (const std::string &command) {
    std::vector<std::string> args;
    std::size_t start, end;
    std::ostringstream responseStream("");
    std::string response("");
    
    // parse arguments
    start = 0;
    end = command.find("|", start);
    while (end != std::string::npos) {
        args.push_back(command.substr(start, end-start));
        start = end + 1;
        end = command.find("|", start);
    }
    args.push_back(command.substr(start));
    
    if (args.size() > 1) {
        if (args[1] == "ping") {
            responseStream << "\n\n" << (commandCount++) << "|pong|" << args[0] << "\n\n";
        } else if (args[1] == "port" && args.size() > 2) {
            int port = 0;
            std::istringstream(args[2]) >> port;
            setNodeState(port);
        }
    }
    
    response = responseStream.str();
    if (response.size() > 0) {
        sendData(response);
    }
    
}

// Called any time the command buffer is modified. Parses the entire buffer,
// removing any full commands. Calls processCommand on each full command.
void parseCommandBuffer() {
    std::size_t i;
    std::string command("");
    
    i = buffer.find("\n\n");
    while (i != std::string::npos) {
        if (i > 0) {
            // every command is prefixed *and* suffixed with "\n\n", so half the time
            // we'll get an empty "command"
            command = buffer.substr(0, i);
            processCommand(command);
        }
        
        // Always erase up to and including the newlines we found, even if it was an
        // empty command.
        buffer.erase(0, i+2);
        
        // Find again
        i = buffer.find("\n\n");
    }
}

// Adds incoming data to the command buffer and then initiates parsing.
void processIncomingData(const std::string &data) {
    if (buffer.max_size() < buffer.size() + data.size()) {
        // If we don't have room to store this data, that means that we have a
        // buffer that has 4 GB of data without any command separators.
        // This indicates that we don't have a command in the buffer, so we can just
        // throw it all out and start over. (This shouldn't ever happen.)
        fprintf(stderr, "Buffer for node stdout exceeded max size (4GB), clearing buffer.");
        buffer = "";
    }
    buffer += data;
    parseCommandBuffer();
}
