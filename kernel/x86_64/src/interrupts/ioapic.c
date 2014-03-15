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
	// Map the I/O APICs to a virtual address below the LAPIC
}

/* All reads/writes must be done in dwords, so 64-bit reads/writes need two
 * 32-bit reads/writes
 *
 * x = 0-Fh
 * y = 0,4,8,Ch
 *
 * FEC0 xy00h - Register select
 * FEC0 xy10h - Data value
 *
 * REG SEL  - 32-bits - [31:8] Reserved - [7:0] - R/W
 * DATA REG - 32-bits - All R/W
 *
 * Address offset is determined by register select bits [0:7]
 *
 * Registers:
 * 0x00 - I/O APIC ID  - R/W
 * 0x01 - I/O APIC VER - RO
 * 0x02 - I/O APIC Arbitration ID - RO
 * 0x10-0x3F - I/O Redirection table entries 0-23 64-bit each - R/W
 *
 * Registers:
 * ID  - [31:28] Reserved - [27:24] ID - [23:0] Reserved
 * VER - [31:24] Reserved - [23:16] Max Redirections - [15:8] Reserved - [7:0] Version
 * ARB - [31:28] Reserved - [27:24] ID - [23:0] Reserved
 *
 * Redirection Registers:
 * Interrupt priority is independent of the physical location (unlike the 8259A)
 * Vector maps to priority, and each interrupt can be assigned a vector
 *
 * [63:56] - Destination field - When mode = physical then [59:56] = APIC ID
 *                             - When mode = logical then [63:56] = Set of processors
 * [55:17] - Reserved
 * [16]    - Interrupt mask - 1 = masked - 0 = not masked
 * [15]    - Trigger mode - 1 = level sensitive - 0 = edge sensitive
 * [14]    - For level intrs only - Set to 1 when 
 * [13]    - Pin polarity - 1 = active high - 0 = active low
 * [12]    - Delivery status - 1 = send pending - 0 = idle
 * [11]    - Destination Mode - 1 = logical mode - 0 = physical mode
 * [10:8]  - Delivery mode - 000 = fixed mode
 *                         - 001 = lowest priority
 *                         - 010 = SMI - edge trigger required
 *                         - 011 = Reserved
 *                         - 100 = NMI - must be programmed as edge triggered
 *                         - 101 = INIT - must be programmed as edge triggered
 *                         - 110 = Reserved
 *                         - 111 = ExtINT - must be programmed as edge triggered
 * [7:0]   - Interrupt vector - Valid range is 0x10-0xFE
 */
void ioapic_write(uint32_t* addr, uint8_t reg_idx, uint32_t val)
{
	*addr = reg_idx;
	*(addr+1) = val;
}

uint32_t ioapic_read(uint32_t* addr, uint8_t reg_idx)
{
	*addr = reg_idx;
	return *(addr+1);
}

