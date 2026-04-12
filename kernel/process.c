#include "process.h"
#include "pmm.h"

static struct process process_table[MAX_PROCESSES];
static uint32_t next_pid = 1;
static struct process *current_process = 0;

struct process *process_create(const char *name, void (*entry)())
{
    /* Find free slot */
    struct process *proc = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_table[i].pid == 0 || process_table[i].state == PROCESS_TERMINATED)
        {
            proc = &process_table[i];
            break;
        }
    }

    if (!proc)
    {
        return 0; /* No free slots */
    }

    /* Allocate stack */
    uint32_t stack = pmm_alloc_frame();
    if (!stack)
    {
        return 0;
    }

    /* Initialize process */
    proc->pid = next_pid++;
    proc->state = PROCESS_READY;
    proc->stack_bottom = stack;
    proc->stack_top = stack + PROCESS_STACK_SIZE;

    /* Copy name */
    for (int i = 0; i < 31 && name[i]; i++)
    {
        proc->name[i] = name[i];
    }
    proc->name[31] = 0;

    /* Set up initial CPU context */
    /* This context will be restored by scheduler on first switch */
    proc->context.eax = 0;
    proc->context.ebx = 0;
    proc->context.ecx = 0;
    proc->context.edx = 0;
    proc->context.esi = 0;
    proc->context.edi = 0;
    proc->context.ebp = 0;
    proc->context.esp = proc->stack_top;
    proc->context.eip = (uint32_t)entry;
    proc->context.eflags = 0x202; /* Interrupts enabled */
    proc->context.cs = 0x08;
    proc->context.ds = 0x10;

    return proc;
}

void process_exit(void)
{
    if (current_process)
    {
        current_process->state = PROCESS_TERMINATED;
        current_process->pid = 0;
        /* Free stack */
        pmm_free_frame(current_process->stack_bottom);
    }

    /* Halt forever */
    while (1)
    {
        __asm__ volatile("hlt");
    }
}

struct process *process_get_current(void)
{
    return current_process;
}

void process_set_current(struct process *proc)
{
    current_process = proc;
}
