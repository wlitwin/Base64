#include "mptables.h"

#include "panic.h"

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
					kprintf("BUS Entry\n");
					kprintf("Bus ID: %d - %l6s\n", be->bus_id, be->bus_type_str);
					type_ptr += sizeof(BusEntry);
				}
				break;
			case IOAPIC_ENT_TYPE:
				{
					const IOAPICEntry* ie = (IOAPICEntry*) type_ptr; 
					kprintf("IOAPIC Entry\n");
					kprintf("ID: %d - VER: %d - Flags: 0x%x\n", ie->id, ie->version, ie->flags);
					kprintf("ADDR: 0x%x\n", ie->address);
					type_ptr += sizeof(IOAPICEntry);
				}
				break;
			case IOINT_ENT_TYPE:
				{
					const IOIntEntry* ie = (IOIntEntry*) type_ptr;
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

MFPStruct* find_mfp_struct()
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
			return (MFPStruct*)&edba[i];
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
				return (MFPStruct*)&read_only[i];
			}
		}
	}

	return NULL;
}
