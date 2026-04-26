#include "scheduler.h"
#include "pmm.h"
#include "kernel.h"

#define MAX_PROCESSES 64
#define STACK_SIZE    PAGE_SIZE   /* one 4 KB page per process */

static pcb_t  pcb_pool[MAX_PROCESSES];
static uint32_t next_pid = 0;

/* Singly-linked circular run queue */
pcb_t* run_queue   = NULL;  /* head of queue */
static pcb_t* current_pcb = NULL;

/* Idle process — runs when nothing else is READY */
static pcb_t* idle_pcb = NULL;

/* ── Idle task ──────────────────────────────────────────────────────────── */
static void idle_task(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

/* ── Forward declarations ───────────────────────────────────────────────── */
static void enqueue(pcb_t* pcb);
static void dequeue(pcb_t* pcb);

/* ── Internal helpers ───────────────────────────────────────────────────── */

static pcb_t* alloc_pcb(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pcb_pool[i].state == PROCESS_DEAD &&
            pcb_pool[i].pid == 0) {
            return &pcb_pool[i];
        }
    }
    /* Find a DEAD slot that has been used before */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pcb_pool[i].state == PROCESS_DEAD) {
            return &pcb_pool[i];
        }
    }
    return NULL;
}

/* Public enqueue for threads (tcb_t cast to pcb_t*) */
void scheduler_enqueue(pcb_t* pcb) {
    enqueue(pcb);
}

/* Public dequeue for thread_delete */
void scheduler_dequeue(pcb_t* pcb) {
    dequeue(pcb);
}

static void enqueue(pcb_t* pcb) {
    pcb->next = NULL;
    if (run_queue == NULL) {
        run_queue = pcb;
        return;
    }
    pcb_t* tail = run_queue;
    while (tail->next != NULL) tail = tail->next;
    tail->next = pcb;
}

static void dequeue(pcb_t* pcb) {
    if (run_queue == pcb) {
        run_queue = pcb->next;
        pcb->next = NULL;
        return;
    }
    pcb_t* prev = run_queue;
    while (prev && prev->next != pcb) prev = prev->next;
    if (prev) {
        prev->next = pcb->next;
        pcb->next = NULL;
    }
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void scheduler_init(void) {
    /* Zero the PCB pool */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        pcb_pool[i].pid        = 0;
        pcb_pool[i].state      = PROCESS_DEAD;
        pcb_pool[i].priority   = 0;
        pcb_pool[i].esp        = 0;
        pcb_pool[i].stack_base = 0;
        pcb_pool[i].next       = NULL;
    }

    /* Create the idle process */
    idle_pcb = create_process(idle_task, 0);
    if (!idle_pcb) kernel_panic("scheduler_init: failed to create idle process");

    /* Idle starts as current so the first schedule() has something to switch from */
    current_pcb = idle_pcb;
    current_pcb->state = PROCESS_RUNNING;

    /* NOTE: interrupts are NOT enabled here — caller must call sti when ready */
}

pcb_t* create_process(void (*entry_fn)(void), uint8_t priority) {
    pcb_t* pcb = alloc_pcb();
    if (!pcb) return NULL;

    void* stack = pmm_alloc_page();
    if (!stack) return NULL;

    /* Set up initial stack frame so context_switch's ret lands at entry_fn.
     * Stack grows downward; stack_top = stack + PAGE_SIZE.
     * We push entry_fn as the return address, then zero-fill for pusha regs.
     */
    uint32_t* sp = (uint32_t*)((uint8_t*)stack + STACK_SIZE);

    /* Simulate the stack frame that context_switch expects:
     *   [esp+0] = return address (entry_fn)
     * context_switch does: mov [old->esp], esp; mov esp, [new->esp]; ret
     * So we just need entry_fn at the top of the new stack.
     */
    sp--;
    *sp = (uint32_t)(uintptr_t)entry_fn;

    pcb->pid        = ++next_pid;
    pcb->state      = PROCESS_READY;
    pcb->priority   = priority;
    pcb->esp        = (uint32_t)(uintptr_t)sp;
    pcb->stack_base = (uint32_t)(uintptr_t)stack;
    pcb->next       = NULL;

    enqueue(pcb);
    return pcb;
}

void schedule(void) {
    if (!current_pcb) return;

    /* Find next READY process after current in the queue */
    pcb_t* next = current_pcb->next;
    if (!next) next = run_queue; /* wrap around */

    /* Walk the queue looking for a READY process */
    pcb_t* start = next;
    while (next && next->state != PROCESS_READY) {
        next = next->next;
        if (!next) next = run_queue;
        if (next == start) { next = NULL; break; } /* full loop, none ready */
    }

    /* Fall back to idle if nothing is READY */
    if (!next || next->state != PROCESS_READY) {
        if (current_pcb == idle_pcb) return; /* already idle */
        next = idle_pcb;
    }

    if (next == current_pcb) return; /* nothing to switch to */

    pcb_t* old = current_pcb;
    if (old->state == PROCESS_RUNNING) old->state = PROCESS_READY;

    next->state = PROCESS_RUNNING;
    current_pcb = next;

    context_switch(old, next);
}

pcb_t* get_current_process(void) {
    return current_pcb;
}

void process_exit(void) {
    pcb_t* dying = current_pcb;
    dying->state = PROCESS_DEAD;
    dequeue(dying);
    pmm_free_page((void*)(uintptr_t)dying->stack_base);
    dying->pid        = 0;
    dying->stack_base = 0;
    dying->esp        = 0;

    /* Switch to idle temporarily; schedule() will pick the next READY process */
    current_pcb = idle_pcb;
    idle_pcb->state = PROCESS_RUNNING;
    context_switch(dying, idle_pcb);
}
