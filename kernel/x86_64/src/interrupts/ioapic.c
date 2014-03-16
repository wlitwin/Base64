#include "ioapic.h"

#include "apic.h"
#include "memory/defines.h"
#include "memory/paging.h"
#include "mptables.h"
#include "inttypes.h"
#include "kprintf.h"
#include "safety.h"
#include "panic.h"

void ioapic_write32(uint32_t ioapic_idx, uint8_t reg_idx, uint32_t val);

void ioapic_write64(uint32_t ioapic_idx, uint8_t reg_idx, uint64_t val);

uint32_t ioapic_read32(uint32_t ioapic_idx, uint8_t reg_idx);

uint64_t ioapic_read64(uint32_t ioapic_idx, uint8_t reg_idx);

// The first I/O APIC is located here, subsequent ones are located
// at 4KiB increments after this address. So the second I/O APIC is
// located at 0xFEC0_1000. Although if an MP configuration table is
// provided then the I/O APIC may reside at a different adress. 
//
// Page 32 of Intel Multi-Processor Specification
// http://download.intel.com/design/pentium/datashts/24201606.pdf
#define IOAPIC1_BASE 0xFEC00000

static uint64_t ioapic_base_virt_address = 0;

#define MASK_INT 0x10000
#define LOWEST_PRIO 0x100
#define ACTIVE_LOW 0x2000

void ioapic_init()
{
	// Check how many I/O APICs are present in the system
	const uint32_t num_ioapics = get_ioapic_entry_count();
	if (num_ioapics == 0)
	{
		panic("No I/O APICs found!");
	}

	kprintf("Found %d I/O APICs\n", num_ioapics);

	// Map the I/O APICs to a virtual address below the LAPIC
	// All of the I/O APICs should be sequential in memory separated
	// in 4KiB increments.
	const IOAPICEntry* ie = get_ioapic_entries();

	uint32_t prev_address = ie[0].address;
	for (uint32_t i = 1; i < num_ioapics; ++i)
	{
		ASSERT(ie[i].address > prev_address);
		ASSERT(ie[i].address - prev_address == 0x1000);

		prev_address = ie[i].address;
	}

	// Make a virtual->physical mapping for the I/O APICs
	uint64_t ioapic_virt_address = APIC_LOCATION - 0x1000*num_ioapics;	
	ioapic_base_virt_address = ioapic_virt_address;
	for (uint32_t i = 0; i < num_ioapics; ++i)
	{
		if (!kmap_page(ioapic_virt_address, ie[i].address,
					PG_FLAG_RW | PG_FLAG_PWT | PG_FLAG_PCD, PAGE_4KIB))
		{
			kprintf("I/O APIC #%d\n", i);
			panic("Failed to map I/O APIC");
		}

		// This assumption was verified earlier
		ioapic_virt_address += 0x1000;
	}

	kprintf("Done mapping I/O APICs\n");

	// Now the I/O APIC interrupts have to be remapped and the 8259A PIC disabled
	// Typical interrupt IRQ priority order (highest -> lowest) is:
	// 0,1,2,8,9,10,11,12,13,14,15,3,4,5,6,7
	// We'll follow that until something else comes up
	ioapic_write64(0, 0x10, 0xFC); // 0
	ioapic_write64(0, 0x12, 0xF4); // 1
	// Apparently IRQ 2 is something internal to the PIC. It generates a lot of
	// interrupts, so we'll disable it for now until more information is found.
	ioapic_write64(0, 0x14, MASK_INT);//0xEC); // 2
	ioapic_write64(0, 0x16, 0xA4); // 3
	ioapic_write64(0, 0x18, 0x9C); // 4
	ioapic_write64(0, 0x1A, 0x94); // 5
	ioapic_write64(0, 0x1C, 0x8C); // 6
	ioapic_write64(0, 0x1E, 0x84); // 7
	ioapic_write64(0, 0x20, 0xE4); // 8
	ioapic_write64(0, 0x22, 0xDC); // 9
	ioapic_write64(0, 0x24, 0xD4); // 10
	ioapic_write64(0, 0x26, 0xCC); // 11
	ioapic_write64(0, 0x28, 0xC4); // 12
	ioapic_write64(0, 0x2A, 0xBC); // 13
	ioapic_write64(0, 0x2C, 0xB4); // 14
	ioapic_write64(0, 0x2E, 0xAC); // 15
	ioapic_write64(0, 0x30, MASK_INT); // 16 - PCI
	ioapic_write64(0, 0x32, MASK_INT); // 17 - PCI
	ioapic_write64(0, 0x34, MASK_INT); // 18 - PCI
	ioapic_write64(0, 0x36, MASK_INT); // 19 - PCI
	ioapic_write64(0, 0x38, MASK_INT); // 20 - Motherboard
	ioapic_write64(0, 0x3A, MASK_INT); // 21 - Motherboard
	ioapic_write64(0, 0x3C, MASK_INT); // 22 - GPI
	ioapic_write64(0, 0x3E, MASK_INT); // 23 - SMI special pin

	kprintf("Done redirecting interrupts\n");
	kprintf("0x%x\n", ioapic_read64(0, 0x10));
}

/* All reads/writes must be done in dwords, so 64-bit reads/writes need two
 * 32-bit reads/writes
 *
 * x = 0-Fh
 * y = 0,4,8,Ch
 *
 * FEC0 xy00h - Register select
 * FEC0 xy10h - Data value
 *
 * REG SEL  - 32-bits - [31:8] Reserved - [7:0] - R/W
 * DATA REG - 32-bits - All R/W
 *
 * Address offset is determined by register select bits [0:7]
 *
 * Registers:
 * 0x00 - I/O APIC ID  - R/W
 * 0x01 - I/O APIC VER - RO
 * 0x02 - I/O APIC Arbitration ID - RO
 * 0x10-0x3F - I/O Redirection table entries 0-23 64-bit each - R/W
 *
 * Registers:
 * ID  - [31:28] Reserved - [27:24] ID - [23:0] Reserved
 * VER - [31:24] Reserved - [23:16] Max Redirections - [15:8] Reserved - [7:0] Version
 * ARB - [31:28] Reserved - [27:24] ID - [23:0] Reserved
 *
 * Redirection Registers:
 * Interrupt priority is independent of the physical location (unlike the 8259A)
 * Vector maps to priority, and each interrupt can be assigned a vector
 *
 * [63:56] - Destination field - When mode = physical then [59:56] = APIC ID
 *                             - When mode = logical then [63:56] = Set of processors
 * [55:17] - Reserved
 * [16]    - Interrupt mask - 1 = masked - 0 = not masked
 * [15]    - Trigger mode - 1 = level sensitive - 0 = edge sensitive
 * [14]    - For level intrs only - Set to 1 when 
 * [13]    - Pin polarity - 1 = active high - 0 = active low
 * [12]    - Delivery status - 1 = send pending - 0 = idle
 * [11]    - Destination Mode - 1 = logical mode - 0 = physical mode
 * [10:8]  - Delivery mode - 000 = fixed mode
 *                         - 001 = lowest priority
 *                         - 010 = SMI - edge trigger required
 *                         - 011 = Reserved
 *                         - 100 = NMI - must be programmed as edge triggered
 *                         - 101 = INIT - must be programmed as edge triggered
 *                         - 110 = Reserved
 *                         - 111 = ExtINT - must be programmed as edge triggered
 * [7:0]   - Interrupt vector - Valid range is 0x10-0xFE
 */
void ioapic_write32(uint32_t ioapic_idx, uint8_t reg_idx, uint32_t val)
{
	uint32_t* addr = (uint32_t*)(ioapic_base_virt_address + ioapic_idx*0x1000);
	*addr = reg_idx;
	*(addr+4) = val;
}

void ioapic_write64(uint32_t ioapic_idx, uint8_t reg_idx, uint64_t val)
{
	ioapic_write32(ioapic_idx, reg_idx+1, (val >> 32) & 0xFFFFFFFF);
	ioapic_write32(ioapic_idx, reg_idx, val & 0xFFFFFFFF);
}

uint32_t ioapic_read32(uint32_t ioapic_idx, uint8_t reg_idx)
{
	uint32_t* addr = (uint32_t*)(ioapic_base_virt_address + ioapic_idx*0x1000);
	*addr = reg_idx;
	return *(addr+4);
}

uint64_t ioapic_read64(uint32_t ioapic_idx, uint8_t reg_idx)
{
	uint64_t out_val = ioapic_read32(ioapic_idx, reg_idx+1);
	out_val <<= 32;
	out_val |= ioapic_read32(ioapic_idx, reg_idx);

	return out_val;
}

