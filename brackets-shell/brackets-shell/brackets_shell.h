/*
 *  brackets_shell.h
 *  brackets-shell
 *
 *  Created by Hegyközi Gergely on 12/5/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

extern "C" {
#pragma GCC visibility push(default)

/* External interface to the brackets_shell, C-based */

CFStringRef brackets_shellUUID(void);

#pragma GCC visibility pop
}
