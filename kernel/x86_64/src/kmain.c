#include "kprintf.h"

void kmain(void)
{
	kprintf("Entered kmain");
	while(1) { __asm__("hlt"); }
}
