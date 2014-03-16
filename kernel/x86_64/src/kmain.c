#include "memory/init.h"
#include "interrupts/init.h"
#include "textmode.h"
#include "kprintf.h"

void kmain(void)
{
	clear_screen();
	kprintf("Entered kmain\n");

	memory_init();
	interrupts_init();

	__asm__("sti");

	while(1) { __asm__("hlt"); }
}
