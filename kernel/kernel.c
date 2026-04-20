#include <stdint.h>
#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "ps2.h"
#include "timer.h"
#include "keyboard.h"
#include "pmm.h"
#include "heap.h"
#include "paging.h"
#include "process.h"
#include "scheduler.h"
#include "shell.h"

/* Forward declaration */
void kernel_main(void);

/* ------------------------------------------------------------------ */
/* BSS boundaries (provided by linker.ld)                             */
/* ------------------------------------------------------------------ */
extern uint8_t _bss_start[];
extern uint8_t _bss_end[];

/* ================================================================== */
/* Serial helpers — available before VGA is initialised               */
/* ================================================================== */
static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port)
{
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}
static inline int serial_empty(void) { return inb(0x3F8 + 5) & 0x20; }
static void serial_putchar(char c)
{
    while (!serial_empty())
        ;
    outb(0x3F8, (uint8_t)c);
}
static void serial_print(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
            serial_putchar('\r');
        serial_putchar(*s++);
    }
}

/* ================================================================== */
/* Background demo threads                                             */
/*                                                                     */
/* These run alongside the shell and print occasional status lines.   */
/* They use scheduler_yield() — NOT hlt — so the shell stays          */
/* responsive and keyboard interrupts are not blocked.                */
/* ================================================================== */

static volatile uint32_t t1_count = 0;
static volatile uint32_t t2_count = 0;

void thread1_entry(void)
{
    while (1)
    {
        t1_count++;
        if (t1_count % 20000u == 0)
        {
            vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
            vga_print("[Thread1] iterations=");
            vga_print_int((int32_t)t1_count);
            vga_putchar('\n');
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        }
        scheduler_yield();
    }
}

void thread2_entry(void)
{
    while (1)
    {
        t2_count++;
        if (t2_count % 30000u == 0)
        {
            vga_set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
            vga_print("[Thread2] iterations=");
            vga_print_int((int32_t)t2_count);
            vga_putchar('\n');
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        }
        scheduler_yield();
    }
}

/* ================================================================== */
/* Shell process entry point                                           */
/*                                                                     */
/* The interactive shell lives in its own process so the scheduler    */
/* can context-switch away from it (e.g. for background threads)      */
/* and back again when a keyboard IRQ wakes it up.                    */
/* ================================================================== */
void shell_process_entry(void)
{
    shell_init();
    shell_run();
    /* shell_run() loops forever, but be tidy if it ever returns */
    process_exit();
}

/* ================================================================== */
/* kernel_entry — first C code called by stage2, before kernel_main  */
/* ================================================================== */
void kernel_entry(void) __attribute__((section(".text.startup"), used));

void kernel_entry(void)
{
    /*
     * Zero the BSS segment BEFORE any C code references global/static
     * variables.  The linker places _bss_start/_bss_end for us.
     */
    for (uint8_t *p = _bss_start; p < _bss_end; p++)
        *p = 0;

    kernel_main();

    /* Should never reach here */
    while (1)
        __asm__ volatile("hlt");
}

/* ================================================================== */
/* kernel_main — hardware init, process creation, scheduler hand-off  */
/* ================================================================== */
void kernel_main(void)
{
    /* ---- VGA + early serial ---- */
    vga_init();
    serial_print("=== MyOS kernel_main ===\n");
    vga_print("Booting MyOS...\n");

    /* ---- GDT ---- */
    /* Stage2 already loaded a flat GDT; we reload our own to be safe */
    gdt_init();
    serial_print("[OK] GDT\n");
    vga_print("[OK] GDT\n");

    /* ---- IDT ---- */
    idt_init();
    serial_print("[OK] IDT\n");
    vga_print("[OK] IDT\n");

    /* ---- PIC ---- */
    pic_init();
    serial_print("[OK] PIC\n");
    vga_print("[OK] PIC\n");

    /* ---- PS/2 controller ---- */
    ps2_controller_init();
    serial_print("[OK] PS/2\n");
    vga_print("[OK] PS/2\n");

    /* ---- PIT timer at 100 Hz ---- */
    timer_init(100);
    serial_print("[OK] Timer @ 100Hz\n");
    vga_print("[OK] Timer @ 100Hz\n");

    /* ---- Keyboard ---- */
    keyboard_init();
    serial_print("[OK] Keyboard\n");
    vga_print("[OK] Keyboard\n");

    /* ---- Physical memory manager ---- */
    pmm_init(32u * 1024u * 1024u); /* 32 MB */
    serial_print("[OK] PMM\n");
    vga_print("[OK] PMM: ");
    vga_print_int((int32_t)pmm_free_frames());
    vga_print(" frames free\n");

    /* ---- Kernel heap ---- */
    heap_init();
    serial_print("[OK] Heap\n");
    vga_print("[OK] Heap\n");

    /* ---- Paging — identity-map 0–16 MB ---- */
    paging_init();
    serial_print("[OK] Paging (identity 0-16MB)\n");
    vga_print("[OK] Paging (identity 0-16MB)\n");

    /* ---- Scheduler ---- */
    scheduler_init();
    serial_print("[OK] Scheduler\n");
    vga_print("[OK] Scheduler\n");

    /* ----------------------------------------------------------------
     * Create all processes BEFORE enabling interrupts.
     * This avoids a timer IRQ firing before the process list is ready.
     * ---------------------------------------------------------------- */
    struct process *t1 = process_create("Thread1", thread1_entry);
    struct process *t2 = process_create("Thread2", thread2_entry);
    struct process *shell = process_create("shell", shell_process_entry);

    if (t1)
    {
        scheduler_add(t1);
        vga_print("[OK] Thread1 created\n");
    }
    else
    {
        vga_print("[!!] Thread1 FAILED\n");
    }

    if (t2)
    {
        scheduler_add(t2);
        vga_print("[OK] Thread2 created\n");
    }
    else
    {
        vga_print("[!!] Thread2 FAILED\n");
    }

    if (shell)
    {
        scheduler_add(shell);
        vga_print("[OK] Shell process created\n");
    }
    else
    {
        vga_print("[!!] Shell process FAILED\n");
        while (1)
            ;
    }

    vga_print("\n");

    /* ---- Enable hardware interrupts ---- */
    __asm__ volatile("sti");
    serial_print("[OK] Interrupts enabled\n");
    vga_print("[OK] Interrupts enabled\n\n");

    /* ----------------------------------------------------------------
     * Hand control to the shell process.
     *
     * scheduler_switch() saves the current (kernel_main) context into
     * a dummy location and restores the shell process context.
     *
     * From this point kernel_main becomes the "idle task": if every
     * other process is blocked/terminated the scheduler returns here
     * and we spin in hlt.
     * ---------------------------------------------------------------- */
    shell->state = PROCESS_RUNNING;
    scheduler_switch(shell);

    /* ---- Idle loop (reached only when all processes finish) ---- */
    serial_print("[IDLE] All processes done, halting.\n");
    vga_print("[IDLE] Nothing to run. System halted.\n");

    __asm__ volatile("cli");
    while (1)
        __asm__ volatile("hlt");
}