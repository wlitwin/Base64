#include "apic.h"

#include "memory/defines.h"
#include "memory/paging.h"
#include "cpuid.h"
#include "panic.h"
#include "safety.h"
#include "kprintf.h"
#include "inttypes.h"
#include "defines.h"
#include "support.h"

// For the 8259 PIC
#define PIC_MASTER_CMD_PORT 0x20
#define PIC_MASTER_IMR_PORT 0x21
#define PIC_SLAVE_CMD_PORT 0xA0
#define PIC_SLAVE_IMR_PORT 0xA1
#define PIC_MASTER_SLAVE_LINE 0x04
#define PIC_SLAVE_ID 0x02
#define PIC_86MODE 0x1
#define PIC_ICW1BASE 0x10
#define PIC_NEEDICW4 0x01
#define PIC_EOI 0x20

#define PIC_REMAP_BASE 0x30

#define APIC_BASE_MSR 0x1B
#define APIC_EN (0x10)
#define LVT_MASK (0x1 << 16)
#define LVT_EXTINT (0x700)
#define LVT_NMI (0x400)
#define LVT_LEVEL_TRIG (0x8000)
#define EOI_IDX (0xB0/sizeof(uint32_t))
#define TIMER_IDX (0x320 / sizeof(uint32_t))
#define LINT0_IDX (0x350 / sizeof(uint32_t))
#define LINT1_IDX (0x360 / sizeof(uint32_t))
#define PERFCNT_IDX (0x340 / sizeof(uint32_t))
#define ERROR_IDX (0x370 / sizeof(uint32_t))
#define SPURIOUS_IDX (0xF0 / sizeof(uint32_t))

#define SPURIOUS_IRQ 0x50

#define APIC_PRESENCE (0x1 << 9)
#define APIC_LOCATION 0xFFFFFFFFFFFFF000
volatile uint32_t* volatile APIC_REGS = (uint32_t*)APIC_LOCATION;

/* Read the APICs starting physical location from the machine
 * specific register.
 */
static 
uint32_t apic_base_address()
{
	uint32_t eax, edx;
	readmsr(APIC_BASE_MSR, &eax, &edx);
	return eax & 0xFFFFF000;
}

static
void redirect_pic()
{
	// ICW1
	_outb(PIC_MASTER_CMD_PORT, PIC_ICW1BASE | PIC_NEEDICW4);		
	_outb(PIC_SLAVE_CMD_PORT, PIC_ICW1BASE | PIC_NEEDICW4);

	// ICW2
	// Master offset to 32
	// Slave offset to 40
	_outb(PIC_MASTER_IMR_PORT, PIC_REMAP_BASE);
	_outb(PIC_SLAVE_IMR_PORT, PIC_REMAP_BASE+0x8);

	// ICW3
	_outb(PIC_MASTER_IMR_PORT, PIC_MASTER_SLAVE_LINE);
	_outb(PIC_SLAVE_IMR_PORT, PIC_SLAVE_ID);

	// ICW4
	_outb(PIC_MASTER_IMR_PORT, PIC_86MODE);
	_outb(PIC_SLAVE_IMR_PORT, PIC_86MODE);

	// OCW1 Allow interrupts on all lines
	_outb(PIC_MASTER_IMR_PORT, 0x0);
	_outb(PIC_SLAVE_IMR_PORT, 0x0);
}

static
void apic_spurious_handler(uint64_t vector, uint64_t code)
{
	kprintf("Spurious: 0x%x - %d\n", vector, code);

	// According to section 10.9 of the Intel manual the spurious
	// interrupt handler should return without an EOI.
}

inline __attribute__((always_inline))
void pic_acknowledge(const uint64_t vector)
{
	if (vector >= PIC_REMAP_BASE && vector < PIC_REMAP_BASE+0x10)
	{
		_outb(PIC_MASTER_CMD_PORT, PIC_EOI);
		if (vector > PIC_REMAP_BASE+0x8)
		{
			_outb(PIC_SLAVE_CMD_PORT, PIC_EOI);
		}
	}
}

inline __attribute__((always_inline))
void apic_eoi()
{
	APIC_REGS[EOI_IDX] = 0;
}

void apic_init()
{
	// Redirect the PIC
	redirect_pic();

	// By default the LAPIC is mapped to the address
	// 0xFEE00000 and should have caching disabled.
	// Don't know if this is sufficient, but seems to work.
	
	// First step is to see if the processor has an APIC.
	// When CPUID is executed with an operand of 1, bit 9
	// of the CPUID feature flags returned in EDX indicates
	// if an LAPIC exists (1 if it does, 0 if not).
	uint32_t eax, edx;	
	cpuid(0x1, &eax, &edx);

	if ((edx & APIC_PRESENCE) == 0)
	{
		panic("No LAPIC present\n");
	}

	const uint32_t apic_addr = apic_base_address();

	if (apic_addr == 0)
	{
		panic("No APIC present");
	}

	kprintf("APIC Physical Address: 0x%x\n", apic_addr);

	// We'll remap this APIC to the highest part of the address
	// space: 0xFFFFFFFFFFFFF000.
	if (!kmap_page(APIC_LOCATION, apic_addr, 
			PG_FLAG_RW | PG_FLAG_PWT | PG_FLAG_PCD, PAGE_4KIB))
	{
		panic("Failed to remap APIC");
	}

	// Try to read the APIC ID from the newly mapped address
	kprintf("APIC ID: 0x%x\n", APIC_REGS[0x20/4]);	
	kprintf("APIC VER: 0x%x\n", APIC_REGS[0x30/4]);	

	// Disable the timer interrupt
	APIC_REGS[TIMER_IDX] = LVT_MASK;
	// When delivery mode is EXTINT it's always level triggered
	APIC_REGS[LINT0_IDX] = LVT_LEVEL_TRIG | LVT_EXTINT;
	// Vector information is ignored with NMI setting
	APIC_REGS[LINT1_IDX] = LVT_NMI;
	APIC_REGS[PERFCNT_IDX] = LVT_MASK;
	APIC_REGS[ERROR_IDX] = LVT_MASK;
	// Set the spurious register while also enabling the APIC
	APIC_REGS[SPURIOUS_IDX] = SPURIOUS_IRQ | APIC_EN;

	APIC_REGS[LINT0_IDX] = LVT_LEVEL_TRIG | LVT_EXTINT;
	APIC_REGS[LINT1_IDX] = LVT_NMI;

	// Install spurious handler
	interrupts_install_isr(SPURIOUS_IRQ, apic_spurious_handler);
}
