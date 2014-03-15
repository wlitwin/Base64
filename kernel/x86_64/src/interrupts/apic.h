#ifndef __X86_64_APIC_H__
#define __X86_64_APIC_H__

#define APIC_LOCATION 0xFFFFFFFFFFFFF000

/* Setup the LAPIC if it exists.
 */
void apic_init(void);

#endif
