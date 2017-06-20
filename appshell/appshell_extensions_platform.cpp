#include "appshell/appshell_extensions_platform.h"
#include <unicode/ucsdet.h>

CharSetDetect::CharSetDetect() {
	charsetDetector_ = NULL;
	icuError = U_ZERO_ERROR;
	charsetDetector_ = ucsdet_open(&icuError);
	if (U_FAILURE(icuError))
		throw "Failed to open detector";
}

CharSetDetect::~CharSetDetect() {
	if (charsetDetector_) {
		ucsdet_close(charsetDetector_);
	}
}

void CharSetDetect::operator()(const char* bufferData, size_t bufferLength, std::string &detectedCharSet) {
	detectedCharSet = "";
	const UCharsetMatch* charsetMatch_;

	// send text
	ucsdet_setText(charsetDetector_, bufferData, bufferLength, &icuError);
	if (U_FAILURE(icuError))
		throw "Failed to set text";

	// detect language
	charsetMatch_ = ucsdet_detect(charsetDetector_, &icuError);
	if (U_FAILURE(icuError))
		throw "Failed to detect CharSet";

	const char* detectedCharsetName = ucsdet_getName(charsetMatch_, &icuError);
	detectedCharSet = detectedCharsetName;

	// Get Language Name
	const char* detectedLanguage = ucsdet_getLanguage(charsetMatch_, &icuError);
	// Get Confidence
	int32_t detectionConfidence = ucsdet_getConfidence(charsetMatch_, &icuError);
}
