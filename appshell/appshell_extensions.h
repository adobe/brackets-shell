
#pragma once

#include "cefclient/client_app.h"
#include "cefclient/client_handler.h"

namespace appshell_extensions {
	
// Delegate creation. Called from ClientApp and ClientHandler.
void CreateProcessMessageDelegates(
								   ClientHandler::ProcessMessageDelegateSet& delegates);
void CreateRequestDelegates(ClientHandler::RequestDelegateSet& delegates);
}  // namespace appshell_extensions
