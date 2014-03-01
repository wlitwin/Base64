#include "ioapic.h"

#include "inttypes.h"
#include "kprintf.h"
#include "safety.h"
#include "panic.h"

// MP Floating Pointer Structure
// This is used for reading the multi-processor features
// of the system and lets us know where the I/O APICs are
typedef struct
{
	uint32_t signature; // Contains the string "_MP_"
	uint32_t phys_addr_ptr;
	uint8_t length;
	uint8_t spec_rev;
	uint8_t checksum;
	uint8_t feature_info1;
	uint8_t feature_info2;
	uint8_t reserved[3];
} MFPStruct;

COMPILE_ASSERT(sizeof(MFPStruct) == 16);

typedef struct
{
	uint32_t signature; // String PCMP
	uint16_t base_length;
	uint8_t spec_rev;
	uint8_t checksum;
	char oem_id[8];
	char product_id[12];
	uint32_t oem_table_ptr;
	uint16_t oem_table_size;
	uint16_t entry_count;
	uint32_t lapic_addr;
	uint16_t ext_table_length;
	uint8_t ext_table_checksum;
	uint8_t reserved;
} MPConfigTable;

COMPILE_ASSERT(sizeof(MPConfigTable) == 44);

#define MP_SIG 0x5F504D5F // The string "_MP_"
#define MC_SIG 0x504D4350 // The string "PCMP"

// The first I/O APIC is located here, subsequent ones are located
// at 4KiB increments after this address. So the second I/O APIC is
// located at 0xFEC0_1000. Although if an MP configuration table is
// provided then the I/O APIC may reside at a different adress. 
//
// Page 32 of Intel Multi-Processor Specification
// http://download.intel.com/design/pentium/datashts/24201606.pdf
#define IOAPIC1_BASE 0xFEC00000


void ioapic_init()
{
	// First region to search is the first kilobyte 
	// of the Extended BIOS Data Area (EDBA). But first
	// the EDBA base address needs to be found, this is
	// typically in address 0x40E
	volatile const uint16_t* bda = (uint16_t*) 0x40E;
	kprintf("EDBA Address: 0x%x\n", *bda);
	volatile const uint32_t* edba = (uint32_t*) ((uint64_t)*bda << 4);
	kprintf("EDBA adjusted: 0x%x\n", edba);

	//assert((uint64_t)edba == 0x9FC00);

	// Scan the first 1KiB for the signature
	const MFPStruct* mfp = NULL;
	for (uint32_t i = 0; i < 256; ++i)
	{
		if (edba[i] == MP_SIG)
		{
			mfp = (MFPStruct*)&edba[i];
			break;
		}
	}

	if (mfp == NULL)
	{
		// TODO - Hopefully this case isn't all that common
		// We need to check the top of system physical memory
		volatile uint16_t* base_mem = (uint16_t*) 0x413;
		kprintf("Base Memory: 0x%x\n", *base_mem);
	}

	// TODO if EDBA is undefined, check top of system base memory
	// Hopefully this case isn't very common

	if (mfp == NULL)
	{
		// Need to check the BIOS read-only memory between 0xE0000 and 0xFFFFF
		volatile const uint32_t* read_only = (uint32_t*) 0xE0000;
		for (uint32_t i = 0; i < 32768; ++i)
		{
			if (read_only[i] == MP_SIG)
			{
				mfp = (MFPStruct*)&read_only[i];
				break;
			}
		}
	}

	if (mfp == NULL)
	{
		panic("Could not find MP Signature");
	}

	kprintf("Found MP Sig: 0x%x\n", mfp);
	kprintf("Sig: 0x%x\n", mfp->signature);
	kprintf("PA Ptr: 0x%x\n", mfp->phys_addr_ptr);
	kprintf("Len: %d\n", mfp->length);
	kprintf("Spec Rev: %d\n", mfp->spec_rev);
	kprintf("Checksum: 0x%x\n", mfp->checksum);
	kprintf("Feature Info 1: 0x%x\n", mfp->feature_info1);
	kprintf("Feature Info 2: 0x%x\n", mfp->feature_info2);

	// If the phys_addr_ptr field is non-zero then there is a 
	// configuration table present. Also if the feature_info1
	// field is zero then an MP configuration table is present.
	if (mfp->phys_addr_ptr != 0)
	{
		MPConfigTable* mct = (MPConfigTable*) (uint64_t) mfp->phys_addr_ptr;
		kprintf("\n\nSig: 0x%x\n", mct->signature);
		kprintf("Tbl Len: %d\n", mct->base_length);
		kprintf("Spec Rev: %d\n", mct->spec_rev);
		kprintf("Checksum: 0x%x\n", mct->checksum);
		kprintf("OEM ID: %l8s\n", mct->oem_id);
		kprintf("PROD ID: %l12s\n", mct->product_id);
		kprintf("OEM Tbl Ptr: 0x%x\n", mct->oem_table_ptr);
		kprintf("OEM Tbl Size: %d\n", mct->oem_table_size);
		kprintf("Entry Count: %d\n", mct->entry_count);
		kprintf("Apic Loc: 0x%x\n", mct->lapic_addr);
		kprintf("Ext Tbl Len: %d\n", mct->ext_table_length);
		kprintf("Ext Tbl Check: %d\n", mct->ext_table_checksum);
	}
}
