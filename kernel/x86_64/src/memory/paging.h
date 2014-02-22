#ifndef __X86_64_MEMORY_PAGING__
#define __X86_64_MEMORY_PAGING__

#include "types.h"
#include "inttypes.h"

/* Defined in prekernel.s
 */
extern PML4_Table kernel_PML4;

/* Defined in prekernel.s
 */
extern PDP_Table kernel_PDPTE;

/* Defined in prekernel.s
 */
extern PD_Table kernel_PDT;

/* Invalidate a virtual address
 */
#define invlpg(X) __asm__ volatile("invlpg %0" :: "m" (X))

/* Identity maps from the start of memory 0x00000000 to end of
 * physical RAM. The end if physical RAM is determined from the
 * largest address stored in the mmap module's mmap_array.
 */
void paging_init(void);

/* Map a virtual address to a physical address. Assumes that the physical
 * address is valid, either coming from the physical allocator, or is a
 * special reserved region of RAM like the VGA memory mapped area.
 *
 * Params:
 *   pml4      - Top most page table structure, the PML4 table
 *   virt_addr - The virtual address
 *   phys_addr - The physical address
 *   flags     - Set page attributes like no execute, or cache disable
 *   page_size - 4KiB, 2MiB, 1GiB (although 1GiB may not be supported)
 *
 * Returns:
 *   1 if successfully mapped the virtual address to the physical address.
 *   0 when no physical memory could be allocated for page tables.
 */
uint8_t map_page(PML4_Table* pml4, uint64_t virt_addr, uint64_t phys_addr,
			uint64_t flags, uint64_t page_size);

/* Same as map_page, but substitutes kernel_PML4 for the pml4 parameter */
#define kmap_page(VADDR, PADDR, FLAGS, PSIZE) \
	map_page(kernel_PML4, (VADDR), (PADDR), (FLAGS), (PSIZE))

/* Map a contiguous range of virtual addresses to a contiguous range of physical
 * addresses. Physical addresses are assumed valid.
 *
 * Params:
 *   pml4      - Top most page table structure, the PML4 table
 *   virt_addr - The virtual address
 *   phys_addr - The physical address
 *   flags     - Set page attributes like no execute, or cache disable
 *   page_size - 4KiB, 2MiB, 1GiB (although 1GiB may not be supported)
 *   num_pages - How many pages to map, total memory size is page_size*num_pages
 *
 * Returns:
 *   1 if successfully mapped the range of virtual addresses. 0 otherwise, this
 *   can happen if physical memory cannot be allocated for page tables.
 */
uint8_t map_page_range(PML4_Table* pml4, uint64_t virt_addr, uint64_t phys_addr,
			uint64_t flags, uint64_t page_size, uint64_t num_pages);

/* Same as map_page_range, but substitutes kernel_PML4 for the pml4 parameter */
#define kmap_page_range(VADDR, PADDR, FLAGS, PSIZE, NPAGES) \
	kmap_page_range(kernel_PML4, (VADDR), (PADDR), (FLAGS), (PSIZE), (NPAGES))

/* Map a virtual address to any physical address. Uses the physical memory
 * allocator to get a valid physical address. If there are no more virtual
 * addresses available a kernel panic is invoked.
 *
 * Params:
 *   pml4      - Top most page table structure, the PML4 table
 *   virt_addr - The virtual address
 *   flags     - Set page attributes like no execute, or cache disable
 *   page_size - 4KiB, 2MiB, 1GiB (although 1GiB may not be supported)
 */
void map_page_auto(PML4_Table* pml4, uint64_t virt_addr, uint64_t flags,
			uint64_t page_size);

/* Same as map_page_auto, but substitutes kernel_PML4 for the pml4 parameter */
#define kmap_page_auto(VADDR, FLAGS, PSIZE) \
	map_page_auto(kernel_PML4, (VADDR), (FLAGS), (PSIZE))

/* Unmaps a virtual address. Does not free the physical backing store,
 * only clears the entry in the table structure.
 *
 * Params:
 *   pml4      - Top most page table structure, the PML4 table
 *   virt_addr - The virtual address
 *
 * Returns:
 *   1 if the virtual address was mapped and then successfully unmapped.
 *   0 if the virtual address was not mapped.
 */
uint8_t unmap_page(PML4_Table* pml4, uint64_t virt_addr);

/* Same as map_page_auto, but substitutes kernel_PML4 for the pml4 parameter */
#define kunmap_page(VADDR) unmap_page(kernel_PML4, (VADDR))

/* Similar to unmap_page(), but will use the physical allocator to
 * free the physical location. Should only be used when the page was
 * mapped with map_page_auto(). Causes a kernel panic if the virtual
 * address is not mapped.
 *
 * Params:
 *   pml4      - Top most page table structure, the PML4 table
 *   virt_addr - The virtual address
 */
void unmap_page_auto(PML4_Table* pml4, uint64_t virt_addr);
 
/* Same as unmap_page_auto, but substitutes kernel_PML4 for the pml4 parameter */
#define kunmap_page_auto(VADDR) unmap_page_auto(kernel_PML4, (VADDR))

/* Determine what physical address a virtual address maps to.  
 *
 * Params:
 *   pml4      - Top most page table structure, the PML4 table
 *   virt_addr - The virtual address
 *   out_phys  - An out parameter, if the virtual address is
 *               mapped then it will contain the physical address
 *
 * Returns:
 *   1 if the virtual address is mapped, 0 otherwise. If 1 is returned
 *   then out_phys will contain the physical address virt_addr is mapped
 *   to.
 */
uint8_t virt_to_phys(PML4_Table* pml4, uint64_t virt_addr, uint64_t* out_phys);

/* Same as map_page_auto, but substitutes kernel_PML4 for the pml4 parameter */
#define kvirt_to_phys(VADDR, POUT) virt_to_phys(kernel_PML4, (VADDR), (POUT))

#endif
