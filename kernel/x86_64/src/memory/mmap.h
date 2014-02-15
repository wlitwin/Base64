#ifndef __X86_64_MMAP_H__
#define __X86_64_MMAP_H__

#include "inttypes.h"

// For use by other parts of the kernel
typedef struct
{
	uint64_t base;
	uint64_t length;
} MemoryMap;

// Stores all of the usable regions of memory
extern MemoryMap mmap_array[];
extern uint32_t mmap_length;

/* Read the memory map provided by the BIOS and populate
 * the mmap_array. The mmap_array will contain usable
 * regions of physical memory.
 */
void mmap_init(void);

#endif
