#include "thread.h"
#include "pmm.h"
#include "kernel.h"

#define MAX_THREADS  64
#define STACK_SIZE   PAGE_SIZE

static tcb_t    tcb_pool[MAX_THREADS];
static uint32_t next_tid = 0;
static int      tcb_pool_initialized = 0;

/* Initialize pool on first use */
static void ensure_pool_init(void) {
    if (tcb_pool_initialized) return;
    for (int i = 0; i < MAX_THREADS; i++) {
        tcb_pool[i].base.state = PROCESS_DEAD;
        tcb_pool[i].base.pid   = 0;
        tcb_pool[i].tid        = 0;
    }
    tcb_pool_initialized = 1;
}

static tcb_t* alloc_tcb(void) {
    ensure_pool_init();
    for (int i = 0; i < MAX_THREADS; i++) {
        if (tcb_pool[i].base.state == PROCESS_DEAD) {
            return &tcb_pool[i];
        }
    }
    return NULL;
}

tcb_t* create_thread(pcb_t* parent, void (*entry_fn)(void)) {
    tcb_t* tcb = alloc_tcb();
    if (!tcb) return NULL;

    void* stack = pmm_alloc_page();
    if (!stack) return NULL;

    /* Set up initial stack frame: push entry_fn as return address */
    uint32_t* sp = (uint32_t*)((uint8_t*)stack + STACK_SIZE);
    sp--;
    *sp = (uint32_t)(uintptr_t)entry_fn;

    tcb->tid                = ++next_tid;
    tcb->parent             = parent;
    tcb->wait_next          = NULL;
    tcb->base.pid           = 0;          /* threads don't have their own PID */
    tcb->base.state         = PROCESS_READY;
    tcb->base.priority      = parent ? parent->priority : 1;
    tcb->base.esp           = (uint32_t)(uintptr_t)sp;
    tcb->base.stack_base    = (uint32_t)(uintptr_t)stack;
    tcb->base.next          = NULL;

    /* Add to the scheduler run queue (cast to pcb_t* — same layout) */
    scheduler_enqueue((pcb_t*)tcb);

    return tcb;
}

void thread_exit(void) {
    process_exit(); /* reuses scheduler's exit path — frees stack, removes from queue */
}

/* Delete a thread by TID — callable from shell */
int thread_delete(uint32_t tid) {
    ensure_pool_init();
    for (int i = 0; i < MAX_THREADS; i++) {
        if (tcb_pool[i].tid == tid &&
            tcb_pool[i].base.state != PROCESS_DEAD) {
            tcb_pool[i].base.state = PROCESS_DEAD;
            /* Remove from run queue */
            extern void scheduler_dequeue(pcb_t* pcb);
            scheduler_dequeue((pcb_t*)&tcb_pool[i]);
            pmm_free_page((void*)(uintptr_t)tcb_pool[i].base.stack_base);
            tcb_pool[i].base.stack_base = 0;
            tcb_pool[i].tid = 0;
            return 1; /* success */
        }
    }
    return 0; /* not found */
}
