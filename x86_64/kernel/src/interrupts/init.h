#ifndef __X86_64_INTERRUPTS_INIT_H__
#define __X86_64_INTERRUPTS_INIT_H__

/* Initialize the interrupt descriptor table, interrupt
 * function pointer table, the task state segment and
 * other interrupt devices.
 */
void interrupts_init(void);

#endif
