#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES 16
#define PROCESS_STACK_SIZE 8192 /* 8 KB per process stack (two PMM frames) */

typedef enum
{
    PROCESS_RUNNING = 0,
    PROCESS_READY = 1,
    PROCESS_BLOCKED = 2,
    PROCESS_TERMINATED = 3
} process_state_t;

/*
 * cpu_context — saved register state for switch_context().
 *
 * switch_context only uses esp and eip for the first activation;
 * thereafter it saves/restores ebx, esi, edi, ebp via the stack frame
 * and updates esp/eip in this struct.
 *
 * Offsets must match switch.asm:
 *   +0  eax   +4  ebx   +8  ecx   +12 edx
 *   +16 esi   +20 edi   +24 ebp   +28 esp
 *   +32 eip   +36 eflags  +40 cs  +44 ds
 */
struct cpu_context
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds;
};

struct process
{
    uint32_t pid;
    process_state_t state;
    struct cpu_context context;
    uint32_t stack_top;    /* high address (grows down) */
    uint32_t stack_bottom; /* low address  (PMM frame)  */
    char name[32];
};

struct process *process_create(const char *name, void (*entry)(void));
void process_exit(void);
struct process *process_get_current(void);
void process_set_current(struct process *proc);

#endif