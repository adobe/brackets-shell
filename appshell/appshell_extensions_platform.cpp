#include "appshell/appshell_extensions_platform.h"
#include <unicode/ucsdet.h>
#include <unicode/ucnv.h>

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
	if (U_FAILURE(error))
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
    int targetLen = ustr.extract(NULL, 0, NULL, status);
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
    if (contents.length() >= 3 && contents.substr(0,3) == UTF8_BOM) {
        contents.erase(0,3);
        preserveBOM = true;
    }
}

