#include "defines.h"

#define CODE_SEG_64 0x10
#define DATA_SEG_64 0x20

#define IDT_SEG_PRESENT (0x1 << 15)

#define IDT_DPL0 (0x0 << 13)
#define IDT_DPL1 (0x1 << 13)
#define IDT_DPL2 (0x2 << 13)
#define IDT_DPL3 (0x3 << 13)

#define IDT_TYPE_INT_GATE (0xE << 7)

typedef struct
{
	uint16_t offset_L;
	uint16_t segment_selector;
	uint8_t ist;
	uint8_t flags;
	uint16_t offset_H;
	uint32_t offset_HH;
	uint32_t reserved;
} __attribute__((packed)) IDT_Gate;

COMPILE_ASSERT(sizeof(IDT_Gate) == 16);

extern IDT_Gate start_idt_64[256];

/* This array holds function pointers to C functions that will
 * perform the work to handle an interrupt.
 */
void (*isr_table[256])(uint64_t vector, uint64_t code);

typedef void (*interrupt_handler)(void);

/* Setup an entry in the interrupt descriptor table. Handling an 
 * interrupt is actually a two step process. The IDT is filled with
 * stubs that call assembly routines that make the stack uniform
 * for all interrupts. This is because some interrupts have error
 * codes and others do not. Interrupt vectors that do not push
 * errors have a dummy code pushed instead.
 *
 * This function should only be called to initially setup the stubs.
 * To add a C function handler interrupts_install_isr() should be
 * used instead.
 *
 * See interrupts.S for the assembly stubs.
 *
 * Params:
 *   index - The interrupt vector to install the stub at
 *   fn_ih - The address of the stub
 *  
 */
static void set_idt_entry(uint64_t index, interrupt_handler fn_ih)
{
	if (index > 255)
	{
		panic("Bad interrupt handler!");
	}

	const uint64_t fn_loc = (uint64_t)fn_ih;

	start_idt_64[index].offset_L = fn_loc & 0xFFFF;
	start_idt_64[index].segment_selector = CODE_SEG_64;
	start_idt_64[index].ist = 0;
	start_idt_64[index].flags = 0xEE;
	start_idt_64[index].offset_H = (fn_loc >> 16) & 0xFFFF;
	start_idt_64[index].offset_HH = (fn_loc >> 32) & 0xFFFFFFFF;
	start_idt_64[index].reserved = 0;
}

void interrupts_install_isr(uint64_t index, void handler(uint64_t, uint64_t))
{
	isr_table[index] = handler;
}

/* Places assembly stubs into the IDT and then installs a dummy handler
 * for all the C function handlers.
 *
 * Params:
 *   default_handler - A default ISR to be used when initializing the
 *                     interrupt handler table.
 */
void setup_isr_table(void default_handler(uint64_t, uint64_t))
{
	extern interrupt_handler isr_stub_table[256];
	for (uint16_t i = 0; i < 256; ++i)
	{
		set_idt_entry(i, isr_stub_table[i]);
		interrupts_install_isr(i, default_handler);
	}
}
