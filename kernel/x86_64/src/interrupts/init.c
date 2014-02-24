#include "init.h"

#include "tss.h"
#include "apic.h"
#include "panic.h"
#include "safety.h"
#include "kprintf.h"
#include "defines.h"
#include "inttypes.h"

/* Default interrupt handler. Prints some information about
 * the interrupt, or if it's a serious interrupt halts.
 */
static
void default_handler(uint64_t vector, uint64_t code)
{
	kprintf("Got interrupt: 0x%x - %d\n", vector, code);
	if (vector == 14)
	{
		uint64_t cr2;
		__asm__ volatile ("mov %%cr2, %%rax" ::: "%rax");
		__asm__ volatile ("mov %%rax, %0" : "=m"(cr2));
		kprintf("CR2: 0x%x\n", cr2);

		panic("Page Fault");
	}

	if (vector == 13)
	{
		panic("GPF!");
	}

	pic_acknowledge(vector);
	apic_eoi();
}

/* Defined in interrupts.c
 */
extern
void setup_isr_table(void default_handler(uint64_t, uint64_t));

/* Initialize interrupts.
 */
void interrupts_init()
{
	tss_init();

	setup_isr_table(default_handler);

	// Initialize the APIC
	// TODO fallback to 8259 if there is no APIC?
	apic_init();
}
