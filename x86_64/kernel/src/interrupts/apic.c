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

#define APIC_BASE_MSR 0x1B
#define APIC_EN (0x100)
#define LVT_MASK (0x10000)
#define LVT_EXTINT (0x700)
#define LVT_NMI (0x400)
#define LVT_LEVEL_TRIG (0x8000)
#define ID_IDX (0x20 / sizeof(uint32_t))
#define EOI_IDX (0xB0 / sizeof(uint32_t))
#define TIMER_IDX (0x320 / sizeof(uint32_t))
#define LINT0_IDX (0x350 / sizeof(uint32_t))
#define LINT1_IDX (0x360 / sizeof(uint32_t))
#define TPR_IDX (0x80 / sizeof(uint32_t))
#define PERFCNT_IDX (0x340 / sizeof(uint32_t))
#define ERROR_IDX (0x370 / sizeof(uint32_t))
#define SPURIOUS_IDX (0xF0 / sizeof(uint32_t))

#define SPURIOUS_IRQ 0x50

#define APIC_PRESENCE (0x1 << 9)
volatile uint32_t* volatile APIC_REGS;

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
void apic_spurious_handler(uint64_t vector, uint64_t code)
{
	kprintf("Spurious: 0x%x - %d\n", vector, code);

	// According to section 10.9 of the Intel manual the spurious
	// interrupt handler should return without an EOI.
}

inline __attribute__((always_inline))
void apic_eoi()
{
	APIC_REGS[EOI_IDX] = 0;
}

// The bootstrap processor (BSP) ID, needed during shutdown as the BSP
// needs to be the last processor shutdown.
static
int8_t bsp_id = -1;

// Page 33 Intel Multi-Processor Specification
// http://download.intel.com/design/pentium/datashts/24201606.pdf

void apic_init()
{
	// Redirect the PIC
	//	redirect_pic();

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

	// Make an identity mapping for the LAPIC
	uint64_t dummy;
	kunmap_page(apic_addr, &dummy);
	if (!kmap_page(apic_addr, apic_addr, 
			PG_FLAG_RW | PG_FLAG_PWT | PG_FLAG_PCD, PAGE_4KIB))
	{
		panic("Failed to remap APIC");
	}
	APIC_REGS = (uint32_t*) (uint64_t) apic_addr;

	// Try to read the APIC ID from the newly mapped address
	kprintf("BSP APIC ID: 0x%x\n", APIC_REGS[ID_IDX]);	
	kprintf("BSP APIC VER: 0x%x\n", APIC_REGS[0x30/4]);	

	// Save the bootstrap processor ID
	bsp_id = APIC_REGS[ID_IDX];

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
	APIC_REGS[TPR_IDX] = 0;

	// Install spurious handler
	interrupts_install_isr(SPURIOUS_IRQ, apic_spurious_handler);
}
