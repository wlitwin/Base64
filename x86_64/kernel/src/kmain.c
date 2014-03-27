#include "memory/init.h"
#include "interrupts/init.h"
#include "textmode.h"
#include "kprintf.h"

extern int __KERNEL_ALL_LO;
extern int __KERNEL_ALL_HI;

void kmain(void)
{
	clear_screen();
	kprintf("Entered kmain\n");
	kprintf("KERNEL ALL LO: 0x%x\n", &__KERNEL_ALL_LO);
	kprintf("KERNEL ALL HI: 0x%x\n", &__KERNEL_ALL_HI);

	memory_init();
	interrupts_init();

	__asm__("sti");

	while(1) { __asm__("hlt"); }
}
