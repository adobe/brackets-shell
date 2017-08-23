#include "appshell/appshell_extensions_platform.h"
#include <unicode/ucsdet.h>
#include <unicode/ucnv.h>
#include <fstream>

#ifdef OS_LINUX
#include "appshell/browser/main_context.h"
#include "appshell/browser/root_window_manager.h"
#include "appshell/browser/root_window_gtk.h"
#endif

#define UTF8_BOM "\xEF\xBB\xBF"

CharSetDetect::CharSetDetect() {
	m_charsetDetector_ = NULL;
	m_icuError = U_ZERO_ERROR;
	m_charsetDetector_ = ucsdet_open(&m_icuError);
	if (U_FAILURE(m_icuError))
		throw "Failed to open detector";
}

CharSetDetect::~CharSetDetect() {
	if (m_charsetDetector_ && U_SUCCESS(m_icuError)) {
		ucsdet_close(m_charsetDetector_);
	}
}

void CharSetDetect::operator()(const char* bufferData, size_t bufferLength, std::string &detectedCharSet) {
	detectedCharSet = "";
    UErrorCode error = U_ZERO_ERROR;
	const UCharsetMatch* charsetMatch_;

	// send text
	ucsdet_setText(m_charsetDetector_, bufferData, bufferLength, &error);
	if (U_FAILURE(error))
		throw "Failed to set text";

	// detect language
	charsetMatch_ = ucsdet_detect(m_charsetDetector_, &error);
	if (U_FAILURE(error) || !charsetMatch_)
		throw "Failed to detect CharSet";

	const char* detectedCharsetName = ucsdet_getName(charsetMatch_, &error);
	detectedCharSet = detectedCharsetName;

	// Get Language Name
	//const char* detectedLanguage = ucsdet_getLanguage(charsetMatch_, &error);
	// Get Confidence
	//int32_t detectionConfidence = ucsdet_getConfidence(charsetMatch_, &error);
}

CharSetEncode::CharSetEncode(std::string encoding) {
    m_status = U_ZERO_ERROR;
    m_conv = ucnv_open(encoding.c_str(), &m_status);
    if (U_FAILURE(m_status)) {
        throw "Unable to open Converter";
    }
}

CharSetEncode::~CharSetEncode() {
    if (m_conv && U_SUCCESS(m_status)) {
        ucnv_close(m_conv);
    }
}

void CharSetEncode::operator()(std::string &contents) {
    UnicodeString ustr(contents.c_str());
    UErrorCode error = U_ZERO_ERROR;
    int targetLen = ustr.extract(NULL, 0, m_conv, error);
    if(error != U_BUFFER_OVERFLOW_ERROR) {
        throw "Unable to convert encoding";
    }
    std::auto_ptr<char> target(new  char[targetLen + 1]());
    error = U_ZERO_ERROR;
    ustr.extract(target.get(), targetLen, m_conv, error);
    target.get()[targetLen] = '\0';
    contents.assign(target.get(), targetLen);
}

#if defined(OS_MACOSX) || defined(OS_LINUX)
void DecodeContents(std::string &contents, const std::string& encoding) {
    UnicodeString ustr(contents.c_str(), encoding.c_str());
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = NULL;
    int targetLen = ustr.extract(NULL, 0, conv, status);
    if(status != U_BUFFER_OVERFLOW_ERROR) {
        throw "Unable to decode contents";
    }
    std::auto_ptr<char> target(new char[targetLen + 1]());
    status = U_ZERO_ERROR;
    ustr.extract(target.get(), targetLen, NULL, status);
    target.get()[targetLen] = '\0';
    if (U_SUCCESS(status)) {
        contents.assign(target.get(), targetLen);
    }
    else {
        throw "Unable to decode contents";
    }
}
#endif

void CheckAndRemoveUTF8BOM(std::string& contents, bool& preserveBOM) {
    if (contents.length() >= 3 && contents.substr(0, 3) == UTF8_BOM) {
        preserveBOM = true;
        contents.erase(0, 3);
    }
}

void CheckForUTF8BOM(const std::string& filename, bool& preserveBOM) {
	try {
		std::ifstream file(filename.c_str());
		int ch1, ch2, ch3;
		ch1 = ch2 = ch3 = 0;
		if (file.good())
			ch1 = file.get();
		if (file.good())
			ch2 = file.get();
		if (file.good())
			ch3 = file.get();
		if (ch1 == 0xef && ch2 == 0xbb && ch3 == 0xbf) {
			preserveBOM = true;
		}
	}
	catch (...) {
	}
}

#ifdef OS_LINUX
// The following routine will get the containing GTK root window, for a browser.
scoped_refptr<client::RootWindowGtk> getRootGtkWindow(CefRefPtr<CefBrowser> browser)
{
    DCHECK(browser);
    scoped_refptr<client::RootWindowGtk> rootGtkWindow;
    if(browser.get() && 
        client::MainContext::Get() && 
        client::MainContext::Get()->GetRootWindowManager()){
        scoped_refptr<client::RootWindow> rootWindow = client::MainContext::Get()->GetRootWindowManager()->GetWindowForBrowser(browser->GetIdentifier());
        if (rootWindow){
            rootGtkWindow = dynamic_cast <client::RootWindowGtk *>(rootWindow.get());    
        }
    }
    return rootGtkWindow;
}

void* getMenuParent(CefRefPtr<CefBrowser>browser)
{
    scoped_refptr<client::RootWindowGtk> rootGtkWindow = getRootGtkWindow(browser);
    if (rootGtkWindow){
        // This returns the underlying vbox. GetWindowHandle() is going to
        // return X11 ref because of which all our routines will break.
        return (void*)(rootGtkWindow->GetContainerHandle());
    } else {
        return NULL;
    }
}

void  InstallMenuHandler(GtkWidget* entry, CefRefPtr<CefBrowser> browser, int tag)
{
    scoped_refptr<client::RootWindowGtk> rootGtkWindow = getRootGtkWindow(browser);
    if (rootGtkWindow){
        // Let the gtk root window handle menu activations.
        rootGtkWindow->InstallMenuHandler(entry, tag);
        rootGtkWindow->Show(client::RootWindowGtk::ShowNormal);
    }
}

#endif
