#include "paging.h"

#include "mmap.h"
#include "klib.h"
#include "panic.h"
#include "kprintf.h"
#include "defines.h"
#include "phys_alloc.h"

#define ENTRY_TO_ADDR(X) ((X) & 0x7FFFFFFFFFFFF000)

#define PML4_INDEX(X) (((X) >> 39) & 0x1FF)
#define PDPT_INDEX(X) (((X) >> 30) & 0x1FF) 
#define PDT_INDEX(X)  (((X) >> 21) & 0x1FF)
#define PT_INDEX(X)   (((X) >> 12) & 0x1FF)

#define PML4E_TO_PDPT(X) ((PDP_Table*)((X) & 0x7FFFFFF000))
#define PDPTE_TO_PDT(X)  ((PD_Table*) ((X) & 0x7FFFFFF000))
#define PDTE_TO_PT(X)    ((P_Table*)  ((X) & 0x7FFFFFF000))

void paging_init()
{
	// Find the highest address, this will be what needs to be
	// identity mapped.
	uint64_t max_addr = 0;
	for (uint32_t i = 0; i < mmap_length; ++i)
	{
		const uint64_t high_address = mmap_array[i].base + 
			mmap_array[i].length;

		if (high_address > max_addr)
		{
			max_addr = high_address;
		}
	}

	// We'll allocate as many 2MiB pages as possible
	max_addr = ALIGN_2MIB(max_addr);

	// The kernel already identity maps the first 1 GIB of RAM
	if (max_addr < _1_GIB)
	{
		// Unmap invalid regions
		uint64_t phys_addr = ALIGN_2MIB(max_addr);
		uint64_t paddr = 0;
		while (phys_addr < _1_GIB)
		{
			kunmap_page(phys_addr, &paddr);
			phys_addr += _2_MIB;
		}
	}
	else
	{
		max_addr -= _1_GIB;
		const uint64_t num_pages = max_addr / _2_MIB;
		// Perform the identity mapping
		kmap_page_range(_1_GIB, _1_GIB, PG_FLAG_RW, PAGE_2MIB, num_pages);
	}

	// Now all of physical RAM is identity mapped
}

uint8_t map_page(PML4_Table* pml4, uint64_t virt_addr, uint64_t phys_addr,
			uint64_t flags, uint64_t page_size)
{
	// Note: The allocations aren't released upon error because
	// they can be reused later, when another address is mapped.
	// A TODO might be to check for that, or add a 'clean' 
	// function that checks for empty entries and frees up the tree.

	ASSERT(page_size == PAGE_4KIB || page_size == PAGE_2MIB);
	flags &= PG_SAFE_FLAGS;

	const uint64_t pml4_index = PML4_INDEX(virt_addr);		
	const uint64_t pdpt_index = PDPT_INDEX(virt_addr);
	const uint64_t pdt_index  = PDT_INDEX(virt_addr);
	const uint64_t pt_index   = PT_INDEX(virt_addr);

	if ((pml4->entries[pml4_index] & PML4_PRESENT) == 0)
	{
		// There is no PDPT table allocated
		PDP_Table* pdp = (PDP_Table*) phys_alloc_4KIB();
		if (pdp == NULL) { return 0; }

		memclr(pdp, sizeof(PDP_Table));

		pml4->entries[pml4_index] = (uint64_t)pdp | PML4_WRITABLE | PML4_PRESENT;

		// TODO - is this necessary here?
		invlpg(pdp);
	}

	PDP_Table* pdp = PML4E_TO_PDPT(pml4->entries[pml4_index]);	
	if ((pdp->entries[pdpt_index] & PDPT_PRESENT) == 0)
	{
		PD_Table* pd = (PD_Table*) phys_alloc_4KIB();
		if (pd == NULL) { return 0; }

		memclr(pd, sizeof(PD_Table));
		pdp->entries[pdpt_index] = (uint64_t)pd | PDPT_WRITABLE | PDPT_PRESENT;

		// TODO - is this necessary here?
		invlpg(pd);
	}

	PD_Table* pd = PDPTE_TO_PDT(pdp->entries[pdpt_index]);

	// Check for an existing mapping
	// Debugging for now, as mapping here might be overriding something else
	// that we don't want to have happen
	if ((pd->entries[pdt_index] & PDT_PAGE_SIZE) > 0 &&
			(pd->entries[pdt_index] & PDT_PRESENT) > 0)
	{
		kprintf("VA: 0x%x - PA: 0x%x\n", virt_addr, phys_addr);
		kprintf("PD Entry: 0x%x\n", pd->entries[pdt_index]);
		panic("Double mapping");
	}

	if (page_size == PAGE_2MIB)
	{
		pd->entries[pdt_index] = 
			(uint64_t)MASK_2MIB(phys_addr) | PDT_PAGE_SIZE | PDT_PRESENT | flags;
	}
	else if (page_size == PAGE_4KIB)
	{
		if ((pd->entries[pdt_index] & PDT_PRESENT) == 0)
		{
			P_Table* pt = (P_Table*) phys_alloc_4KIB();
			if (pt == NULL) { return 0; }

			memclr(pt, sizeof(P_Table));
			pd->entries[pdt_index] = (uint64_t)pt | PDT_WRITABLE | PDT_PRESENT | flags;

			// TODO - is this necessary here?
			invlpg(pd);
		}

		P_Table* pt = PDTE_TO_PT(pd->entries[pdt_index]);
		if ((pt->entries[pt_index] & PT_PRESENT) > 0)
		{
			kprintf("VA: 0x%x - PA: 0x%x\n", virt_addr, phys_addr);
			kprintf("PT Entry: 0x%x\n", pt->entries[pt_index]);
			panic("Double mapping");
		}

		pt->entries[pt_index] = (uint64_t)MASK_4KIB(phys_addr) | PT_PRESENT | flags;
	}
	else
	{
		panic("Invalid page size specified");
	}

	invlpg(virt_addr);

	return 1;
}

uint8_t map_page_range(PML4_Table* pml4, uint64_t virt_addr, uint64_t phys_addr,
			uint64_t flags, uint64_t page_size, uint64_t num_pages)
{
	ASSERT(page_size == PAGE_4KIB || page_size == PAGE_2MIB);
	uint64_t add_amt = (page_size == PAGE_4KIB) ? _4_KIB : _2_MIB;
	for (uint64_t i = 0; i < num_pages; ++i)
	{
		uint8_t result = map_page(pml4, virt_addr, phys_addr, flags, page_size);
		if (result != 1)
		{
			return result;
		}
		phys_addr += add_amt;
	}

	return 1;
}

void map_page_auto(PML4_Table* pml4, uint64_t virt_addr, uint64_t flags,
			uint64_t page_size)
{
	uint8_t ret = 0;
	if (page_size == PAGE_4KIB)
	{
		const uint64_t phys_addr = (uint64_t)phys_alloc_4KIB_safe("map_page_auto_4KIB");
		ret = map_page(pml4, virt_addr, phys_addr, flags, page_size);
	}
	else if (page_size == PAGE_2MIB)
	{
		const uint64_t phys_addr = (uint64_t)phys_alloc_2MIB_safe("map_page_auto_2MIB");
		ret = map_page(pml4, virt_addr, phys_addr, flags, page_size);
	}
	else
	{
		panic("Invalid page size specified");
	}

	if (!ret)
	{
		panic("Failed to map page");
	}
}

static 
uint8_t private_unmap_page(PML4_Table* pml4, uint64_t virt_addr, 
			uint64_t* phys_addr, uint64_t* page_type)
{
	// Calculate the entries
	const uint64_t pml4_index = PML4_INDEX(virt_addr);
	const uint64_t pdpt_index = PDPT_INDEX(virt_addr);
	const uint64_t pdt_index  = PDT_INDEX(virt_addr);

	if ((pml4->entries[pml4_index] & PML4_PRESENT) == 0)
	{
		return 0;
	}

	PDP_Table* pdp = PML4E_TO_PDPT(pml4->entries[pml4_index]);
	if ((pdp->entries[pdpt_index] & PDPT_PRESENT) == 0)
	{
		return 0;
	}

	PD_Table* pd = PDPTE_TO_PDT(pdp->entries[pdpt_index]);
	if ((pd->entries[pdt_index] & PDT_PRESENT) == 0)
	{
		return 0;
	}

	if ((pd->entries[pdt_index] & PDT_PAGE_SIZE) > 0)
	{
		// 2MIB region, mask the address
		*phys_addr = ENTRY_TO_ADDR(pd->entries[pdt_index]);
		*page_type = PAGE_2MIB;
		pd->entries[pdt_index] = 0;
	}
	else
	{
		const uint64_t pt_index = PT_INDEX(virt_addr);

		// 4KIB region
		P_Table* pt = PDTE_TO_PT(pd->entries[pdt_index]);
		if ((pt->entries[pt_index] & PT_PRESENT) == 0)
		{
			return 0;
		}

		*phys_addr = ENTRY_TO_ADDR(pt->entries[pt_index]);
		*page_type = PAGE_2MIB;
		pt->entries[pt_index] = 0;
	}

	// TODO - need invlpg?

	return 1;
}

uint8_t unmap_page(PML4_Table* pml4, uint64_t virt_addr, uint64_t* phys_addr)
{
	uint64_t page_type = 0;
	return private_unmap_page(pml4, virt_addr, phys_addr, &page_type);
}

void unmap_page_auto(PML4_Table* pml4, uint64_t virt_addr)
{
	uint64_t phys_addr = 0;
	uint64_t page_type = 0;
	const uint8_t ret = private_unmap_page(pml4, virt_addr, &phys_addr, &page_type);
	if (!ret) { return; }

	if (page_type == PAGE_2MIB)
	{
		phys_free_2MIB((void*)phys_addr);	
	}
	else if (page_type == PAGE_4KIB)
	{
		phys_free_4KIB((void*)phys_addr);
	}
	else
	{
		panic("Unhandled case");
	}
}

uint8_t virt_to_phys(PML4_Table* pml4, uint64_t virt_addr, uint64_t* out_phys)
{
	// Calculate all of the offsets
	const uint64_t pml4_index = PML4_INDEX(virt_addr);
	const uint64_t pdpt_index = PDPT_INDEX(virt_addr);
	const uint64_t pdt_index  = PDT_INDEX(virt_addr);
	const uint64_t pt_index   = PT_INDEX(virt_addr);

	const uint64_t pml4_entry = pml4->entries[pml4_index];
	if ((pml4_entry & PML4_PRESENT) > 0)
	{
		const PDP_Table* const pdp = PML4E_TO_PDPT(pml4_entry);
		const uint64_t pdpt_entry = pdp->entries[pdpt_index];

		if ((pdpt_entry & PDPT_PRESENT) > 0)
		{
			const PD_Table* const pd = PDPTE_TO_PDT(pdpt_entry);
			const uint64_t pdt_entry = pd->entries[pdt_index];

			if ((pdt_entry & PDT_PRESENT) > 0)
			{
				if ((pdt_entry & PDT_PAGE_SIZE) > 0)
				{
					*out_phys = ENTRY_TO_ADDR(pdt_entry) + (virt_addr & 0xFFF);
					return 1;
				}
				else
				{
					const P_Table* const pt = PDTE_TO_PT(pdt_entry);
					const uint64_t pt_entry = pt->entries[pt_index];

					if ((pt_entry & PT_PRESENT) > 0)
					{
						*out_phys = ENTRY_TO_ADDR(pt_entry) + (virt_addr & 0xFFF);
					}
				}
			}
		}
	}

	return 0;
}
