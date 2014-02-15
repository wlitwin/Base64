#include "memory/mmap.h"
#include "textmode.h"
#include "kprintf.h"

void kmain(void)
{
	clear_screen();
	kprintf("Entered kmain\n");

	mmap_init();

	while(1) { __asm__("hlt"); }
}
