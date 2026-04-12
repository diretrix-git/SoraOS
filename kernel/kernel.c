#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "pmm.h"
#include "paging.h"
#include "process.h"
#include "scheduler.h"
#include "shell.h"

static volatile int thread1_ticks = 0;
static volatile int thread2_ticks = 0;

void thread1_entry(void) {
    while (1) {
        thread1_ticks++;
        if (thread1_ticks % 100 == 0) {
            vga_print("[Thread1] Running... ticks=");
            vga_print_int(thread1_ticks);
            vga_print("\n");
        }
        __asm__ volatile ("hlt");
    }
}

void thread2_entry(void) {
    while (1) {
        thread2_ticks++;
        if (thread2_ticks % 150 == 0) {
            vga_print("[Thread2] Running... ticks=");
            vga_print_int(thread2_ticks);
            vga_print("\n");
        }
        __asm__ volatile ("hlt");
    }
}

void kernel_main(void) {
    vga_init();
    vga_print("Booting MyOS...\n");
    
    gdt_init();
    vga_print("GDT loaded\n");
    
    idt_init();
    vga_print("IDT loaded\n");
    
    pic_init();
    vga_print("PIC initialized\n");
    
    timer_init(100);
    vga_print("Timer started (100Hz)\n");
    
    keyboard_init();
    vga_print("Keyboard ready\n");
    
    pmm_init(32 * 1024 * 1024);
    vga_print("PMM ready: ");
    vga_print_int(pmm_free_frames());
    vga_print(" frames free\n");
    
    paging_init();
    vga_print("Paging enabled\n");
    
    scheduler_init();
    vga_print("Scheduler ready\n");
    
    __asm__ volatile ("sti");
    vga_print("Interrupts enabled\n");
    
    vga_print("Creating demo threads...\n");
    struct process* t1 = process_create("Thread1", thread1_entry);
    struct process* t2 = process_create("Thread2", thread2_entry);
    
    if (t1) scheduler_add(t1);
    if (t2) scheduler_add(t2);
    
    vga_print("\n");
    
    shell_init();
    shell_run();
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}
