#include "mptables.h"

#include "panic.h"

static MFPStruct* mfp_struct = NULL;

static const ProcEntry* proc_entries = NULL;
static uint32_t proc_entry_count = 0;

static const BusEntry* bus_entries = NULL;
static uint32_t bus_entry_count = 0;

static const IOAPICEntry* ioapic_entries = NULL;
static uint32_t ioapic_entry_count = 0;

static const IOIntEntry* ioint_entries = NULL;
static uint32_t ioint_entry_count = 0;

static const LAPICIntEntry* lapicint_entries = NULL;
static uint32_t lapicint_entry_count = 0;

/* Parse the entries of a Multi-Processor Configuration Table. Entries
 * follow the table header and are of variable length. The entry_count
 * field of the MPConfigTable structure denotes how many entries to
 * parse. The first byte of each entry defines the entry type. All
 * entries are in ascending order sorted on the entry type.
 *
 * See Page 4-6 (42) in
 * http://download.intel.com/design/pentium/datashts/24201606.pdf
 *
 * Currently there are 5 defined entry types:
 *
 *   Type 0 - Processor - 20 bytes - One entry for every processor
 *   Type 1 - Bus       -  8 bytes - One entry per bus
 *   Type 2 - I/O APIC  -  8 bytes - One entry per I/O APIC
 *   Type 3 - I/O Interrupt Assignment - 8 bytes - One per interrupt source
 *   Type 4 - Local Interrupt Assignment - 8 bytes - One per system interrupt source
 */
static
void parse_mct(const MPConfigTable* mct)
{
	const uint16_t entries = mct->entry_count;	
	const uint8_t* type_ptr = (uint8_t*) (mct + 1);
	for (uint32_t i = 0; i < entries; ++i)
	{
		kprintf("\nEntry: %d - Type: %d\n", i, *type_ptr);
		switch (*type_ptr)
		{
			case PROC_ENT_TYPE:
				{
					const ProcEntry* pe = (ProcEntry*) type_ptr;
					if (proc_entries == NULL)
					{
						proc_entries = pe;
					}
					++proc_entry_count;
					kprintf("Processor\n");
					kprintf("LAPIC: %d - VER: %d\n", pe->lapic_id, pe->lapic_ver);
					kprintf("CPU FLAGS: 0x%x\n", pe->cpu_flags);
					kprintf("CPU SIG: 0x%x\n", pe->cpu_signature);
					kprintf("Feature Flags: 0x%x\n", pe->feature_flags);
					type_ptr += sizeof(ProcEntry);
				}
				break;
			case BUS_ENT_TYPE:
				{
					const BusEntry* be = (BusEntry*) type_ptr;
					if (bus_entries == NULL)
					{
						bus_entries = be;
					}
					++bus_entry_count;
					kprintf("BUS Entry\n");
					kprintf("Bus ID: %d - %l6s\n", be->bus_id, be->bus_type_str);
					type_ptr += sizeof(BusEntry);
				}
				break;
			case IOAPIC_ENT_TYPE:
				{
					const IOAPICEntry* ie = (IOAPICEntry*) type_ptr; 
					if (ioapic_entries == NULL)
					{
						ioapic_entries = ie;
					}
					++ioapic_entry_count;
					kprintf("IOAPIC Entry\n");
					kprintf("ID: %d - VER: %d - Flags: 0x%x\n", ie->id, ie->version, ie->flags);
					kprintf("ADDR: 0x%x\n", ie->address);
					type_ptr += sizeof(IOAPICEntry);
				}
				break;
			case IOINT_ENT_TYPE:
				{
					const IOIntEntry* ie = (IOIntEntry*) type_ptr;
					if (ioint_entries == NULL)
					{
						ioint_entries = ie;
					}
					++ioint_entry_count;
					kprintf("IOInt Entry\n");
					kprintf("Int Type: %d - Flags: 0x%x\n", ie->interrupt_type, ie->flags);
					kprintf("SRC BUS: %d - SRC BUS IRQ: %d\n", ie->src_bus_id, ie->src_bus_irq);
					kprintf("DST IO APIC: %d - DST IO INT: %d\n", ie->dst_ioapic_id, ie->dst_ioapic_int);
					type_ptr += sizeof(IOIntEntry);
				}
				break;
			case LAPIC_ENT_TYPE:
				{
					const LAPICIntEntry* le = (LAPICIntEntry*) type_ptr;
					if (lapicint_entries == NULL)
					{
						lapicint_entries = le;
					}
					++lapicint_entry_count;
					kprintf("LAPIC Entry\n");
					kprintf("INT type: %d - Flags: 0x%x\n", le->int_type, le->flags);
					kprintf("SRC BUS: %d - SRC BUS IRQ: %d\n", le->src_bus_id, le->src_bus_irq);
					kprintf("DST LAPIC: %d - DST LAPIC INT: %d\n", le->dst_lapic_id, le->dst_lapic_int);
					type_ptr += sizeof(LAPICIntEntry);
				}
				break;
			default:
				panic("Unhandled case");
				break;
		}
	}
}

bool find_mfp_struct()
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
	mfp_struct = NULL;

	// Scan the first 1KiB for the signature
	for (uint32_t i = 0; i < 256; ++i)
	{
		if (edba[i] == MP_SIG)
		{
			mfp_struct = (MFPStruct*)&edba[i];
			break;
		}
	}

	if (mfp_struct == NULL)
	{
		// TODO - Hopefully this case isn't all that common
		// We need to check the top of system physical memory
		volatile uint16_t* base_mem = (uint16_t*) 0x413;
		kprintf("Base Memory: 0x%x\n", *base_mem);
	}

	// TODO if EDBA is undefined, check top of system base memory
	// Hopefully this case isn't very common

	if (mfp_struct == NULL)
	{
		// Need to check the BIOS read-only memory between 0xE0000 and 0xFFFFF
		volatile const uint32_t* read_only = (uint32_t*) 0xE0000;
		for (uint32_t i = 0; i < 32768; ++i)
		{
			if (read_only[i] == MP_SIG)
			{
				mfp_struct = (MFPStruct*)&read_only[i];
				break;
			}
		}
	}

	if (mfp_struct != NULL)
	{
		kprintf("Found MP Sig: 0x%x\n", mfp_struct);
		kprintf("Sig: 0x%x\n", mfp_struct->signature);
		kprintf("PA Ptr: 0x%x\n", mfp_struct->phys_addr_ptr);
		kprintf("Len: %d\n", mfp_struct->length);
		kprintf("Spec Rev: %d\n", mfp_struct->spec_rev);
		kprintf("Checksum: 0x%x\n", mfp_struct->checksum);
		kprintf("Feature Info 1: 0x%x\n", mfp_struct->feature_info1);
		kprintf("Feature Info 2: 0x%x\n", mfp_struct->feature_info2);

		// If the phys_addr_ptr field is non-zero then there is a 
		// configuration table present. Also if the feature_info1
		// field is zero then an MP configuration table is present.
		if (mfp_struct->phys_addr_ptr != 0)
		{
			MPConfigTable* mct = (MPConfigTable*) (uint64_t) mfp_struct->phys_addr_ptr;
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

			return true;
		}
		else
		{
			kprintf("Failed to find configuration table!\n");
		}
	}

	return false;
}

const ProcEntry* get_proc_entries() 
{ 
	return proc_entries; 
}

uint32_t get_proc_entry_count() 
{ 
	return proc_entry_count; 
}

const BusEntry* get_bus_entries() 
{ 
	return bus_entries; 
}

uint32_t get_bus_entry_count() 
{ 
	return bus_entry_count; 
}

const IOAPICEntry* get_ioapic_entries()
{
	return ioapic_entries;
}

uint32_t get_ioapic_entry_count()
{
	return ioapic_entry_count;
}

const IOIntEntry* get_ioint_entries()
{
	return ioint_entries;
}

uint32_t get_ioint_entry_count()
{
	return ioint_entry_count;
}

const LAPICIntEntry* get_lapicint_entry()
{
	return lapicint_entries;
}

uint32_t get_lapicint_entry_count()
{
	return lapicint_entry_count;
}
