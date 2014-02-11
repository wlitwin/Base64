#include "kprintf.h"

void panic(const char* message)
{
	kprintf("%s - PANIC  \n", message);
	__asm__ volatile ("cli");
	while (1)
	{
		__asm__ volatile ("hlt");
	}
}
