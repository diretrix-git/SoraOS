#ifndef THREAD_H
#define THREAD_H

#include "types.h"
#include "scheduler.h"

/* Thread Control Block — shares the scheduler run queue with PCBs.
 * The scheduler only reads esp, state, and next (same offsets as pcb_t
 * when we cast tcb_t* to pcb_t*).  We achieve this by embedding a pcb_t
 * as the first member so the offsets are identical.
 */
typedef struct tcb {
    /* Must be first — scheduler casts tcb_t* to pcb_t* */
    pcb_t        base;       /* pid, state, priority, esp, stack_base, next */

    uint32_t     tid;
    pcb_t*       parent;     /* owning process */
    struct tcb*  wait_next;  /* next in a mutex wait queue */
} tcb_t;

/* Create a kernel thread belonging to parent; returns TCB or NULL */
tcb_t* create_thread(pcb_t* parent, void (*entry_fn)(void));

/* Mark current thread DEAD and reschedule */
void   thread_exit(void);

/* Delete a thread by TID from outside; returns 1 on success, 0 if not found */
int    thread_delete(uint32_t tid);

#endif /* THREAD_H */
