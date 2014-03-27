#ifndef __X86_64_INTERRUPTS_PIC_H__
#define __X86_64_INTERRUPTS_PIC_H__

#include "support.h"
#include "inttypes.h"

// For the 8259 PIC
#define PIC_MASTER_CMD_PORT 0x20
#define PIC_MASTER_IMR_PORT 0x21
#define PIC_SLAVE_CMD_PORT 0xA0
#define PIC_SLAVE_IMR_PORT 0xA1
#define PIC_MASTER_SLAVE_LINE 0x04
#define PIC_SLAVE_ID 0x02
#define PIC_86MODE 0x1
#define PIC_ICW1BASE 0x10
#define PIC_NEEDICW4 0x01
#define PIC_EOI 0x20

#define PIC_REMAP_BASE 0x30

/* Masks all of the 8259A's interrupts.
 */
void disable_pic(void);

/* Acknowledge a PIC interrupt.
 */
inline __attribute__((always_inline))
void pic_acknowledge(const uint64_t vector)
{
	if (vector >= PIC_REMAP_BASE && vector < PIC_REMAP_BASE+0x10)
	{
		_outb(PIC_MASTER_CMD_PORT, PIC_EOI);
		if (vector > PIC_REMAP_BASE+0x8)
		{
			_outb(PIC_SLAVE_CMD_PORT, PIC_EOI);
		}
	}
}


#endif
