#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

typedef enum {
    PROCESS_READY   = 0,
    PROCESS_RUNNING = 1,
    PROCESS_BLOCKED = 2,
    PROCESS_DEAD    = 3
} process_state_t;

typedef struct pcb {
    uint32_t         pid;
    process_state_t  state;
    uint8_t          priority;
    uint32_t         esp;         /* saved stack pointer */
    uint32_t         stack_base;  /* base of allocated stack page */
    struct pcb*      next;        /* next in run queue */
} pcb_t;

/* Initialise scheduler, create idle process, enable interrupts */
void   scheduler_init(void);

/* Create a new process; returns PCB or NULL on allocation failure */
pcb_t* create_process(void (*entry_fn)(void), uint8_t priority);

/* Select and switch to the next READY process (called from timer IRQ) */
void   schedule(void);

/* Return the currently running PCB */
pcb_t* get_current_process(void);

/* Mark current process DEAD, free its stack, and reschedule */
void   process_exit(void);

/* ASM context switch — defined in context_switch.asm */
void   context_switch(pcb_t* old_pcb, pcb_t* new_pcb);

/* Used by thread.c to add a TCB to the run queue */
void   scheduler_enqueue(pcb_t* pcb);

/* Exposed for shell ps/threads commands */
extern pcb_t* run_queue;

#endif /* SCHEDULER_H */
