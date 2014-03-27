#ifndef __X86_64_INTERRUPTS_TSS_H__
#define __X86_64_INTERRUPTS_TSS_H__

/* Initialize the Task-State-Segment needed for 64-bit operation.
 * There will only be one used, and will most likely not be changed
 * after this initialization process as different privilege levels
 * won't be used.
 */
void tss_init(void);

#endif
