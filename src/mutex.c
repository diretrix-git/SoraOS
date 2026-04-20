#include "mutex.h"
#include "scheduler.h"

/* Atomic test-and-set using XCHG (implicit LOCK prefix on x86) */
static inline uint32_t test_and_set(volatile uint32_t* lock) {
    uint32_t old = 1;
    __asm__ volatile("xchgl %0, %1" : "+r"(old), "+m"(*lock));
    return old; /* returns previous value: 0 = was free, 1 = was held */
}

void mutex_init(mutex_t* m) {
    m->lock    = 0;
    m->waiters = NULL;
}

void mutex_lock(mutex_t* m) {
    while (test_and_set(&m->lock) != 0) {
        /* Lock was held — block the current thread */
        pcb_t* cur = get_current_process();
        if (cur) {
            cur->state = PROCESS_BLOCKED;
            /* Push onto mutex wait list (cast pcb_t* to tcb_t* — safe if it's a thread) */
            tcb_t* tcb = (tcb_t*)cur;
            tcb->wait_next = m->waiters;
            m->waiters = tcb;
        }
        schedule();
        /* After waking, retry the test-and-set */
    }
}

void mutex_unlock(mutex_t* m) {
    m->lock = 0;
    /* Wake one waiting thread */
    if (m->waiters) {
        tcb_t* waiter = m->waiters;
        m->waiters = waiter->wait_next;
        waiter->wait_next = NULL;
        waiter->base.state = PROCESS_READY;
    }
}
