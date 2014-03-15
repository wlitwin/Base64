#include "ioapic.h"

#include "mptables.h"
#include "inttypes.h"
#include "kprintf.h"
#include "safety.h"
#include "panic.h"

// The first I/O APIC is located here, subsequent ones are located
// at 4KiB increments after this address. So the second I/O APIC is
// located at 0xFEC0_1000. Although if an MP configuration table is
// provided then the I/O APIC may reside at a different adress. 
//
// Page 32 of Intel Multi-Processor Specification
// http://download.intel.com/design/pentium/datashts/24201606.pdf
#define IOAPIC1_BASE 0xFEC00000

const MFPStruct* mfps;


void ioapic_init()
{
	mfps = find_mfp_struct();

	if (mfps == NULL)
	{
		panic("Could not find MP Signature");
	}

	kprintf("Found MP Sig: 0x%x\n", mfps);
	kprintf("Sig: 0x%x\n", mfps->signature);
	kprintf("PA Ptr: 0x%x\n", mfps->phys_addr_ptr);
	kprintf("Len: %d\n", mfps->length);
	kprintf("Spec Rev: %d\n", mfps->spec_rev);
	kprintf("Checksum: 0x%x\n", mfps->checksum);
	kprintf("Feature Info 1: 0x%x\n", mfps->feature_info1);
	kprintf("Feature Info 2: 0x%x\n", mfps->feature_info2);

	// If the phys_addr_ptr field is non-zero then there is a 
	// configuration table present. Also if the feature_info1
	// field is zero then an MP configuration table is present.
	if (mfps->phys_addr_ptr != 0)
	{
		MPConfigTable* mct = (MPConfigTable*) (uint64_t) mfps->phys_addr_ptr;
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

		parse_mct(mct);
	}
}

