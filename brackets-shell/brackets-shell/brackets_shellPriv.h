/*
 *  brackets_shellPriv.h
 *  brackets-shell
 *
 *  Created by Hegyközi Gergely on 12/5/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#include <CoreFoundation/CoreFoundation.h>

#pragma GCC visibility push(hidden)

class Cbrackets_shell {
	public:
		CFStringRef UUID(void);
};

#pragma GCC visibility pop
