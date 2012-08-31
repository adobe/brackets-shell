#pragma once

// Application name used in native code. This name is *not* used in resources.

#ifdef OS_WIN
#define APP_NAME L"Brackets"
#endif 
#ifdef OS_MACOSX
#define APP_NAME @"Brackets"
#endif
