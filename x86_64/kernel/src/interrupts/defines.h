#ifndef __X86_64_INTERRUPTS_DEFINES_H__
#define __X86_64_INTERRUPTS_DEFINES_H__

#include "inttypes.h"

/* Acknowledge an APIC interrupt.
 */
extern void apic_eoi(void);

/* Acknowledge a PIC interrupt.
 *
 * Params:
 *   vector - The vector from the ISR parameter
 */
extern void pic_acknowledge(uint64_t vector);

/* Install a new interrupt handler for the given vector.
 *
 * Params:
 *   index   - What vector to install the ISR at
 *   handler - A pointer to the function that will service
 *             the interrupt.
 */
extern void interrupts_install_isr(uint64_t index, 
		void handler(uint64_t, uint64_t));

#endif
