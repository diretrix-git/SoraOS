#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES 16
#define PROCESS_STACK_SIZE 4096

typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

struct cpu_context {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds;
};

struct process {
    uint32_t pid;
    process_state_t state;
    struct cpu_context context;
    uint32_t stack_top;
    uint32_t stack_bottom;
    char name[32];
};

struct process* process_create(const char* name, void (*entry)());
void process_exit(void);
struct process* process_get_current(void);
void process_set_current(struct process* proc);

#endif
