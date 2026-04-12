#include "scheduler.h"
#include "vga.h"

static struct process* process_queue[MAX_PROCESSES];
static int process_count = 0;
static int current_index = 0;
static struct process* current_process = 0;

void scheduler_init(void) {
    process_count = 0;
    current_index = 0;
    current_process = 0;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_queue[i] = 0;
    }
}

void scheduler_add(struct process* proc) {
    if (process_count >= MAX_PROCESSES) {
        return;
    }
    
    process_queue[process_count++] = proc;
}

void scheduler_tick(void) {
    if (process_count == 0) {
        return;
    }
    
    /* Round robin: move to next process */
    current_index = (current_index + 1) % process_count;
    
    struct process* next_proc = process_queue[current_index];
    
    if (next_proc == current_process || next_proc->state != PROCESS_READY) {
        return;
    }
    
    scheduler_switch(next_proc);
}

struct process* scheduler_get_current(void) {
    return current_process;
}

void scheduler_switch(struct process* proc) {
    if (!current_process) {
        current_process = proc;
        process_set_current(proc);
        return;
    }
    
    if (current_process == proc) {
        return;
    }
    
    switch_context(&current_process->context, &proc->context);
    
    current_process = proc;
    process_set_current(proc);
}
