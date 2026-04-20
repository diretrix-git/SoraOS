#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

/*
 * Scheduler — preemptive round-robin with cooperative yield.
 *
 * scheduler_tick()  — called from the timer IRQ every N ticks.
 *                     Performs a preemptive context switch to the
 *                     next READY process.
 *
 * scheduler_yield() — called voluntarily by a process to give up
 *                     the CPU.  The process remains READY and will
 *                     be switched back to in normal rotation.
 */

void scheduler_init(void);
void scheduler_add(struct process *proc);
void scheduler_remove(struct process *proc);
void scheduler_tick(void);
void scheduler_yield(void);
struct process *scheduler_get_current(void);
void scheduler_switch(struct process *proc);

/* ------------------------------------------------------------------ */
/* Assembly context-switch primitive (switch.asm)                     */
/* ------------------------------------------------------------------ */
void switch_context(struct cpu_context *old_ctx,
                    struct cpu_context *new_ctx);

#endif