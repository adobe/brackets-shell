/*
 *  brackets_shell.cp
 *  brackets-shell
 *
 *  Created by Hegyközi Gergely on 12/5/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#include "brackets_shell.h"
#include "brackets_shellPriv.h"

CFStringRef brackets_shellUUID(void) {
	Cbrackets_shell* theObj = new Cbrackets_shell;
	return theObj->UUID();
}

CFStringRef Cbrackets_shell::UUID() {
	return CFSTR("0001020304050607");
}
