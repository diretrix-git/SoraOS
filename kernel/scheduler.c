#include "scheduler.h"
#include "process.h"
#include "vga.h"

/* ------------------------------------------------------------------ */
/* Serial helpers                                                      */
/* ------------------------------------------------------------------ */
static inline void sched_outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t sched_inb(uint16_t port)
{
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}
static inline int sched_serial_empty(void) { return sched_inb(0x3F8 + 5) & 0x20; }
static void sched_serial_print(const char *s)
{
    while (*s)
    {
        while (!sched_serial_empty())
            ;
        if (*s == '\n')
            sched_outb(0x3F8, '\r');
        sched_outb(0x3F8, (uint8_t)*s++);
    }
}

/* ------------------------------------------------------------------ */
/* Scheduler state                                                     */
/* ------------------------------------------------------------------ */

#define SCHED_MAX 16

static struct process *queue[SCHED_MAX];
static int count = 0;
static int cur_idx = -1;
static struct process *current_process = 0;

/*
 * We keep a "switched_in" flag to guard against re-entrant switches
 * that can happen if a timer IRQ fires inside scheduler_switch itself.
 */
static volatile int switching = 0;

/* ------------------------------------------------------------------ */
void scheduler_init(void)
{
    sched_serial_print("[SCHED] Init\n");
    count = 0;
    cur_idx = -1;
    current_process = 0;
    switching = 0;
    for (int i = 0; i < SCHED_MAX; i++)
        queue[i] = 0;
    sched_serial_print("[SCHED] Done\n");
}

/* ------------------------------------------------------------------ */
void scheduler_add(struct process *proc)
{
    if (!proc || count >= SCHED_MAX)
        return;
    queue[count++] = proc;
    sched_serial_print("[SCHED] Added: ");
    sched_serial_print(proc->name);
    sched_serial_print("\n");
}

/* ------------------------------------------------------------------ */
void scheduler_remove(struct process *proc)
{
    if (!proc)
        return;
    for (int i = 0; i < count; i++)
    {
        if (queue[i] == proc)
        {
            /* Swap with last slot and shrink */
            queue[i] = queue[--count];
            queue[count] = 0;
            /* Fix cur_idx if we displaced it */
            if (cur_idx >= count)
                cur_idx = count - 1;
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/*
 * next_ready() — find the next READY process starting one step past
 * cur_idx, wrapping around.  Returns NULL if no process is ready.
 */
static struct process *next_ready(void)
{
    if (count == 0)
        return 0;

    int start = (cur_idx < 0) ? 0 : cur_idx;

    for (int i = 0; i < count; i++)
    {
        int idx = (start + 1 + i) % count;
        struct process *p = queue[idx];
        if (p && p->state == PROCESS_READY)
        {
            cur_idx = idx;
            return p;
        }
    }

    /*
     * No other process is ready.  If the current process is still
     * runnable, keep running it rather than spinning in the idle hlt.
     */
    if (current_process &&
        (current_process->state == PROCESS_RUNNING ||
         current_process->state == PROCESS_READY))
    {
        return current_process;
    }

    return 0; /* genuinely nothing to run */
}

/* ------------------------------------------------------------------ */
/*
 * scheduler_tick() — called from the timer IRQ.
 * Performs a preemptive round-robin switch.
 * Safe to call from interrupt context.
 */
void scheduler_tick(void)
{
    if (switching)
        return; /* guard against re-entrance */
    if (count == 0)
        return;

    struct process *next = next_ready();
    if (!next || next == current_process)
        return;

    /* Demote current to READY before switching away */
    if (current_process && current_process->state == PROCESS_RUNNING)
        current_process->state = PROCESS_READY;

    next->state = PROCESS_RUNNING;
    scheduler_switch(next);
}

/* ------------------------------------------------------------------ */
/*
 * scheduler_yield() — cooperative yield from a running process.
 * The process is set to READY and we immediately pick the next one.
 */
void scheduler_yield(void)
{
    if (switching)
        return;
    if (!current_process)
        return;

    /* Mark the yielding process as ready (not running) */
    if (current_process->state == PROCESS_RUNNING)
        current_process->state = PROCESS_READY;

    struct process *next = next_ready();

    if (!next || next == current_process)
    {
        /* Nothing else to run — stay running ourselves */
        if (current_process)
            current_process->state = PROCESS_RUNNING;
        return;
    }

    next->state = PROCESS_RUNNING;
    scheduler_switch(next);
}

/* ------------------------------------------------------------------ */
struct process *scheduler_get_current(void)
{
    return current_process;
}

/* ------------------------------------------------------------------ */
/*
 * scheduler_switch() — low-level switch to `proc`.
 *
 * CRITICAL: update current_process BEFORE calling switch_context so
 * that both sides see the correct value after the switch.
 */
void scheduler_switch(struct process *proc)
{
    if (!proc)
        return;

    switching = 1;

    /* ---- First-ever switch: no old context to save ---- */
    if (!current_process)
    {
        current_process = proc;
        process_set_current(proc);
        switching = 0;
        struct cpu_context dummy = {0};
        switch_context(&dummy, &proc->context);
        /* Never returns here on first switch */
        return;
    }

    if (current_process == proc)
    {
        switching = 0;
        return;
    }

    struct process *old = current_process;
    current_process = proc;
    process_set_current(proc);
    switching = 0;

    switch_context(&old->context, &proc->context);
    /*
     * Execution resumes here when `old` is switched back in.
     * current_process already points to `old` because scheduler_switch
     * updated it before calling switch_context to get here.
     */
}