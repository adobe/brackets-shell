#include "appshell/appshell_extensions_platform.h"
#include <unicode/ucsdet.h>

void GetCharsetMatch(const char* bufferData, size_t bufferLength, std::string &detectedCharSet) {
	detectedCharSet = "";
	const UCharsetMatch* charsetMatch_;
	UErrorCode icuError = U_ZERO_ERROR;

	UCharsetDetector* charsetDetector_ = ucsdet_open(&icuError);
	if (U_FAILURE(icuError))
		throw "Failed to open detector";

	// send text
	ucsdet_setText(charsetDetector_, bufferData, bufferLength, &icuError);
	if (U_FAILURE(icuError))
		throw "Failed to set text";

	// detect language
	charsetMatch_ = ucsdet_detect(charsetDetector_, &icuError);
	if (U_FAILURE(icuError))
		throw "Failed to detect charset";

	const char* detectedCharsetName = ucsdet_getName(charsetMatch_, &icuError);
	detectedCharSet = detectedCharsetName;

	// Get Language Name
	const char* detectedLanguage = ucsdet_getLanguage(charsetMatch_, &icuError);
	// Get Confidence
	int32_t detectionConfidence = ucsdet_getConfidence(charsetMatch_, &icuError);
	ucsdet_close(charsetDetector_);
}