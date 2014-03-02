#ifndef __X86_64_PANIC_H__
#define __X86_64_PANIC_H__

#include "macro.h"
#include "kprintf.h"

/* A panic that includes the location of the error. Call panic_ for regular
 * panic.
 */
#define panic(X) \
	{ kprintf("%s ", __FILE__ ":" S__LINE__); panic_(X); }

/* Plain panic that does not include line and file information.
 */
void panic_(const char* message);

#endif
