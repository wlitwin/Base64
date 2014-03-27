#include "mmap.h"
#include "panic.h"
#include "safety.h"
#include "kprintf.h"

// Symbol placed by the linker
// We assume that these two locations will encompass all code
// data and stacks needed by the kernel for initialization.
extern uint64_t __KERNEL_ALL_LO;
extern uint64_t __KERNEL_ALL_HI;

// Actual addresses of the symbols
const uint64_t KERNEL_ALL_LO_ADDR = (uint64_t)&__KERNEL_ALL_LO;
const uint64_t KERNEL_ALL_HI_ADDR = (uint64_t)&__KERNEL_ALL_HI;

// The maximum possible number of mmap entries
#define MMAP_MAX_ENTRIES ((0x7C00 - 0x2D04) / 24)

// Location of the MMapEntry array
#define MMAP_ADDRESS 0x2D04

// Location of the MMapEntry array length
#define MMAP_COUNT_ADDRESS 0x2D00

#define kprintf(...)

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

// TODO - dynamically allocate this somewhere, not hard code the number of entries
#define MAX_ENTRIES 10
MemoryMap mmap_array[MAX_ENTRIES];
int32_t mmap_length = 0;

void mmap_init()
{
	kprintf("Entering MMAP\n");

	const MMapEntry* mmap = (MMapEntry*)MMAP_ADDRESS;
	const uint32_t mmap_count = *(uint32_t*)MMAP_COUNT_ADDRESS;

	for (uint32_t i = 0; i < mmap_count; ++i)
	{
		kprintf("(%d) Base: 0x%x Length: %d type: %d ACPI: %d\n", 
				i, mmap[i].base, mmap[i].length, mmap[i].type, mmap[i].ACPI);	
	}

	// This is checked by the assert above
	const uint64_t reserved_lo = KERNEL_ALL_LO_ADDR;
	const uint64_t reserved_hi = KERNEL_ALL_HI_ADDR;

	// Need to fix the MMAP entries
	mmap_length = 0;
	for (uint32_t i = 0; i < mmap_count && mmap_length < MAX_ENTRIES; ++i)
	{
		if (mmap[i].type != TYPE_USABLE) continue;

		// We have some (possibly invalid) assumptions about the memory map
		// given by the BIOS. The first is that every region is unique, no
		// duplicate or overlapping regions. Second is that the type fields
		// are accurate.
		
		mmap_array[mmap_length].base = mmap[i].base;
		mmap_array[mmap_length].length = mmap[i].length;
		++mmap_length;
	}

	for (int32_t i = 0; i < mmap_length; ++i)
	{
		kprintf("0x%x - %d\n", mmap_array[i].base, mmap_array[i].length);
	}

	if (mmap_length == 0)
	{
		panic("Failed to find suitable usable memory region.");	
	}

	kprintf("Res Lo: 0x%x - Res Hi: 0x%x\n", reserved_lo, reserved_hi);
	kprintf("MMAP Entries: %d\n", mmap_length);

	// Fix all the found regions
	for (int32_t i = 0; i < mmap_length; ++i)
	{
		// Fix the region in case it intersects the kernel's memory
		uint64_t base = mmap_array[i].base;
		uint64_t end = base + mmap_array[i].length;

		kprintf("Region: 0x%x - 0x%x\n", base, end);

		if (base < reserved_lo && end > reserved_hi)
		{
			// TODO split region, but maybe not because it intersects kernel?
			base = reserved_hi;
		}
		else if (base < reserved_hi && end > reserved_hi)
		{
			base = reserved_hi;
		}
		else if ((base >= reserved_lo && end <= reserved_hi) ||
 				 // Don't allow regions below the kernel
				 (base < reserved_lo && end <= reserved_hi))
		{
			kprintf("case 3\n");
			// Remove this entry by moving down all other entries
			for (int32_t j = i; j < mmap_length-1; ++j)
			{
				mmap_array[j] = mmap_array[j+1];
			}
			--mmap_length;
			--i; // We want start at this new moved entry
			continue; // Skip the assignment below
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

		// Region does not intersect kernel
		mmap_array[i].base = base;
		mmap_array[i].length = end - base;

		kprintf("Fixed region: 0x%x - %d - %dMiB\n", 
				mmap_array[i].base, mmap_array[i].length,
				mmap_array[i].length / 1024 / 1024);
	}

	if (mmap_length == 0)
	{
		panic("No suitable regions after fixing");
	}
}
