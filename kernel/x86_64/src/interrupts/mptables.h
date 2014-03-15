#ifndef __X86_64_INTERRUPTS_MPTABLES_H__
#define __X86_64_INTERRUPTS_MPTABLES_H__

#include "safety.h"
#include "inttypes.h"

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
#define PROC_ENT_TYPE   0
#define BUS_ENT_TYPE    1
#define IOAPIC_ENT_TYPE 2
#define IOINT_ENT_TYPE  3
#define LAPIC_ENT_TYPE  4

typedef struct
{
	uint8_t entry_type;
	uint8_t lapic_id;
	uint8_t lapic_ver;
	uint8_t cpu_flags;
	uint32_t cpu_signature;
	uint32_t feature_flags;
	uint32_t reserved[2];
} ProcEntry;

COMPILE_ASSERT(sizeof(ProcEntry) == 20);

typedef struct
{
	uint8_t entry_type;
	uint8_t bus_id;
	char bus_type_str[6];
} BusEntry;

COMPILE_ASSERT(sizeof(BusEntry) == 8);

typedef struct
{
	uint8_t entry_type;
	uint8_t id;
	uint8_t version;
	uint8_t flags;
	uint32_t address;
} IOAPICEntry;

COMPILE_ASSERT(sizeof(IOAPICEntry) == 8);

typedef struct
{
	uint8_t entry_type;
	uint8_t interrupt_type;
	uint8_t flags;
	uint8_t reserved;
	uint8_t src_bus_id;
	uint8_t src_bus_irq;
	uint8_t dst_ioapic_id;
	uint8_t dst_ioapic_int;
} IOIntEntry;

COMPILE_ASSERT(sizeof(IOIntEntry) == 8);

typedef struct
{
	uint8_t entry_type;
	uint8_t int_type;
	uint8_t flags;
	uint8_t reserved;
	uint8_t src_bus_id;
	uint8_t src_bus_irq;
	uint8_t dst_lapic_id;
	uint8_t dst_lapic_int;
} LAPICIntEntry;

COMPILE_ASSERT(sizeof(LAPICIntEntry) == 8);

/* The Multi-Processor Floating Pointer Structure contains information
 * about the multi-processor configuration of the system. It is used
 * (among other things) to find the I/O APICs. The first I/O APIC default
 * location is 0xFEC00000, but there may be more than one or it may
 * have been moved, so it's best to double check.
 *
 * This function goes through the four possible locations of the MFPS
 * and searches for the _MP_ signature. It will return the location of
 * the MFPS if found, or NULL if none of the locations contained the
 * signature.
 */
MFPStruct* find_mfp_struct(void);

/* Parse a Multi-Processor Configuration Table.
 *
 * For now it dumps debug information.
 */
void parse_mct(const MPConfigTable* mct);

#endif
