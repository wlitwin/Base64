#include "init.h"
#include "mmap.h"
#include "paging.h"
#include "phys_alloc.h"

void memory_init()
{
	mmap_init();
	setup_physical_allocator();
	paging_init();
}
