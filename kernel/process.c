#include "process.h"
#include "pmm.h"
#include "scheduler.h"

/* ------------------------------------------------------------------ */
/* Serial helpers (used during early init before scheduler is ready)  */
/* ------------------------------------------------------------------ */
static inline void outb_proc(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb_proc(uint16_t port)
{
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}
static inline int proc_serial_empty(void) { return inb_proc(0x3F8 + 5) & 0x20; }
static void proc_serial_print(const char *s)
{
    while (*s)
    {
        while (!proc_serial_empty())
            ;
        if (*s == '\n')
            outb_proc(0x3F8, '\r');
        outb_proc(0x3F8, (uint8_t)*s++);
    }
}

/* ------------------------------------------------------------------ */

static struct process process_table[MAX_PROCESSES];
static uint32_t next_pid = 1;
static struct process *current_process = 0;

/* ------------------------------------------------------------------ */
struct process *process_create(const char *name, void (*entry)(void))
{
    /* Find a free slot */
    struct process *proc = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_table[i].pid == 0 ||
            process_table[i].state == PROCESS_TERMINATED)
        {
            proc = &process_table[i];
            break;
        }
    }
    if (!proc)
    {
        proc_serial_print("[PROC] No free slot\n");
        return 0;
    }

    /*
     * Allocate two contiguous PMM frames for an 8 KB stack.
     * We allocate the second frame right after the first; if the PMM
     * doesn't guarantee contiguity just use one frame (4 KB) — still
     * safe for our simple processes.
     */
    uint32_t stack_phys = pmm_alloc_frame();
    if (!stack_phys)
    {
        proc_serial_print("[PROC] No memory for stack\n");
        return 0;
    }
    /* Try for a second frame to make an 8 KB stack */
    uint32_t stack_phys2 = pmm_alloc_frame();
    uint32_t stack_size;
    if (stack_phys2 == stack_phys + 4096)
    {
        stack_size = 8192;
    }
    else
    {
        /* Non-contiguous or failed — use 4 KB, free the second */
        if (stack_phys2)
            pmm_free_frame(stack_phys2);
        stack_size = 4096;
    }

    proc->pid = next_pid++;
    proc->state = PROCESS_READY;
    proc->stack_bottom = stack_phys;
    proc->stack_top = stack_phys + stack_size;

    /* Copy name */
    int i;
    for (i = 0; i < 31 && name[i]; i++)
        proc->name[i] = name[i];
    proc->name[i] = 0;

    /*
     * Build the initial stack frame that switch_context will restore.
     *
     * switch_context (switch.asm) does:
     *   1. Pushes ebp, ebx, esi, edi  onto the OLD stack (save)
     *   2. Pops  ebp, ebx, esi, edi  from the NEW context
     *   3. Loads new esp from new_ctx->esp
     *   4. Pushes new_ctx->eip, then ret  → jumps to entry point
     *
     * So for a brand-new process we need:
     *   new_ctx->esp  = top of stack MINUS four zero callee-save words
     *   new_ctx->eip  = entry point
     *
     * The four words on the stack represent the callee-save frame that
     * switch_context will pop into ebp/ebx/esi/edi (all zero = fine).
     */
    uint32_t *sp = (uint32_t *)proc->stack_top;

    *(--sp) = 0; /* ebp will be popped from here */
    *(--sp) = 0; /* ebx */
    *(--sp) = 0; /* esi */
    *(--sp) = 0; /* edi */

    proc->context.esp = (uint32_t)sp;
    proc->context.eip = (uint32_t)entry;

    /* Remaining context fields — informational only */
    proc->context.eax = 0;
    proc->context.ebx = 0;
    proc->context.ecx = 0;
    proc->context.edx = 0;
    proc->context.esi = 0;
    proc->context.edi = 0;
    proc->context.ebp = 0;
    proc->context.eflags = 0x202; /* IF=1, reserved bit set */
    proc->context.cs = 0x08;
    proc->context.ds = 0x10;

    return proc;
}

/* ------------------------------------------------------------------ */
void process_exit(void)
{
    if (current_process)
    {
        current_process->state = PROCESS_TERMINATED;
        /*
         * Free the stack frames.  If we had 8 KB (two frames), we need
         * to free both; check whether stack_top - stack_bottom == 8192.
         */
        uint32_t stack_size = current_process->stack_top - current_process->stack_bottom;
        pmm_free_frame(current_process->stack_bottom);
        if (stack_size > 4096)
            pmm_free_frame(current_process->stack_bottom + 4096);

        current_process->pid = 0;
    }

    /*
     * Yield back to the scheduler — do NOT hlt here.
     * scheduler_yield() will pick the next READY process.
     * If no process is ready the scheduler returns immediately
     * and we fall through to the idle hlt loop.
     */
    scheduler_yield();

    /* Should never be reached, but defensive hlt just in case */
    while (1)
        __asm__ volatile("hlt");
}

/* ------------------------------------------------------------------ */
struct process *process_get_current(void) { return current_process; }
void process_set_current(struct process *p) { current_process = p; }