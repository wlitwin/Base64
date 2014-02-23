#include "apic.h"

#include "memory/defines.h"
#include "memory/paging.h"
#include "cpuid.h"
#include "panic.h"
#include "safety.h"
#include "kprintf.h"
#include "inttypes.h"

#define APIC_BASE_MSR 0x1B

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

void apic_init()
{
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
}
