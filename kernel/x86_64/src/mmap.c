#include "mmap.h"
#include "panic.h"
#include "safety.h"
#include "kprintf.h"

// Symbol placed by the linker
extern uint64_t __KERNEL_START; // Lower address
extern uint64_t __KERNEL_END;   // Higher address

extern uint64_t __KERNEL_STACK_START; // Higher address
extern uint64_t __KERNEL_STACK_END;   // Lower address

// Actual addresses of the symbols
const uint64_t KERNEL_LO_ADDR = (uint64_t)&__KERNEL_START;
const uint64_t KERNEL_HI_ADDR = (uint64_t)&__KERNEL_END;

const uint64_t KERNEL_STACK_HI_ADDR = (uint64_t)&__KERNEL_STACK_START;
const uint64_t KERNEL_STACK_LO_ADDR = (uint64_t)&__KERNEL_STACK_END;

// The maximum possible number of mmap entries
#define MMAP_MAX_ENTRIES ((0x7C00 - 0x2D04) / 24)

// Location of the MMapEntry array
#define MMAP_ADDRESS 0x2D04

// Location of the MMapEntry array length
#define MMAP_COUNT_ADDRESS 0x2D00

// Constants for the type field in the MMapEntry structure
#define TYPE_USABLE 1
#define TYPE_RESERVED 2
#define TYPE_ACPI_RECLAIMABLE 3
#define TYPE_ACPI_NVS 4
#define TYPE_BAD_MEMORY 5
#define TYPE_NOT_USABLE 6
// Other values are undefined

// What the BIOS memory map function returns
typedef struct
{
	uint64_t base;
	uint64_t length;
	uint32_t type; // 1 - Usable RAM
	               // 2 - Reserved
				   // 3 - ACPI Reclaimable
				   // 4 - ACPI NVS
				   // 5 - Bad Memory
	uint32_t ACPI;
} MMapEntry;

COMPILE_ASSERT(sizeof(MMapEntry) == 24);

// For now we only support one entry, but can be expanded later
MemoryMap mmap_array[1];
uint32_t mmap_length = 1;

void mmap_init()
{
	kprintf("Entering MMAP\n");
	// We're going to assume that the kernel stack is directly below the
	// kernel's code and data. This allows us to treat it as one large
	// region below when fixing the memory map regions.
	ASSERT(KERNEL_LO_ADDR == KERNEL_STACK_HI_ADDR);

	const MMapEntry* mmap = (MMapEntry*)MMAP_ADDRESS;
	const uint32_t mmap_count = *(uint32_t*)MMAP_COUNT_ADDRESS;

	kprintf("KS: 0x%x - KE: 0x%x\nKSS: 0x%x - KSE: 0x%x\n", 
			KERNEL_HI_ADDR, KERNEL_LO_ADDR, KERNEL_STACK_HI_ADDR, KERNEL_STACK_LO_ADDR);

	for (uint32_t i = 0; i < mmap_count; ++i)
	{
		kprintf("(%d) Base: 0x%x Length: %d type: %d ACPI: %d\n", 
				i, mmap[i].base, mmap[i].length, mmap[i].type, mmap[i].ACPI);	
	}

	// This is checked by the assert above
	const uint64_t reserved_lo = KERNEL_STACK_LO_ADDR;
	const uint64_t reserved_hi = KERNEL_HI_ADDR;

	uint32_t largest_region = 0xFFFFFFFF;

	// Need to fix the MMAP entries
	for (uint32_t i = 0; i < mmap_count; ++i)
	{
		if (mmap[i].type != TYPE_USABLE) continue;

		// We have some (possibly invalid) assumptions about the memory map
		// given by the BIOS. The first is that every region is unique, no
		// duplicate or overlapping regions. Second is that the type fields
		// are accurate.

		// For now to make life easy, we're going to look for the largest
		// mmap entry. There should be a large contiguous segment that is
		// pretty close to the amount of physical machine memory. This can
		// easily be expanded at a later time to make use of other available
		// regions.
		if (largest_region == 0xFFFFFFFF
				|| mmap[i].length > mmap[largest_region].length)
		{
			largest_region = i;	
		}
	}

	if (largest_region == 0xFFFFFFFF)
	{
		panic("Failed to find suitable usable memory region.");	
	}

	kprintf("Found region: %d - 0x%x - %d - %dMiB\n", largest_region, 
			mmap[largest_region].base, mmap[largest_region].length,
			mmap[largest_region].length / 1024 / 1024);

	// Fix the region in case it intersects the kernel's memory
	uint64_t base = mmap[largest_region].base;
	uint64_t end = mmap[largest_region].length;

	// We need to split in two, but we'll ignore entries below 1MiB
	// for now, because that area needs special treatment anyway.
	if (base < reserved_lo && end > reserved_hi)
	{
		// XXX - May need to change at future date to split region
		base = reserved_hi;
	}
	else if (base >= reserved_lo && end > reserved_hi)
	{
		base = reserved_hi;
	}
	else if (base >= reserved_lo && end <= reserved_hi)
	{
		panic("Region is fully inside the kernel.");
	}
	else if (base < reserved_lo && end <= reserved_hi)
	{
		end = reserved_lo;
	}
	else
	{
		panic("Unhandled case");
	}

	// Perform some more sanity checks
	if (base <= reserved_lo || end <= reserved_lo)
	{
		panic("Base region is too low");
	}

	if (end <= base)
	{
		panic("Region does not exist");
	}

	// Update the array
	mmap_array[0].base = base;
	mmap_array[0].length = end - base;

	kprintf("Final region: 0x%x - %d - %dMiB\n", 
			mmap_array[0].base, mmap_array[0].length,
			mmap_array[0].length / 1024 / 1024);
}
