#include "appshell/appshell_extensions.h"

namespace appshell_extensions {

namespace {
class ProcessMessageDelegate : public ClientHandler::ProcessMessageDelegate {
public:
	ProcessMessageDelegate()
	{
	}

	// From ClientHandler::ProcessMessageDelegate.
	virtual bool OnProcessMessageRecieved(
										  CefRefPtr<ClientHandler> handler,
										  CefRefPtr<CefBrowser> browser,
										  CefProcessId source_process,
										  CefRefPtr<CefProcessMessage> message) OVERRIDE {
		std::string message_name = message->GetName();
		
		return false;
	}
	
	IMPLEMENT_REFCOUNTING(ProcessMessageDelegate);
};
	
} // namespace
	
void CreateProcessMessageDelegates(
								   ClientHandler::ProcessMessageDelegateSet& delegates) {
	delegates.insert(new ProcessMessageDelegate);
}

void CreateRequestDelegates(ClientHandler::RequestDelegateSet& delegates) {
//	delegates.insert(new RequestDelegate);
}
	
} // namespace appshell_extensions
