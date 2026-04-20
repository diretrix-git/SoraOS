#include "idt.h"
#include "vga.h"
#include "timer.h"
#include "scheduler.h"
#include "keyboard.h"

#define IDT_ENTRIES 256

/* Human-readable exception names for the panic screen */
static const char *exception_messages[] = {
    "Division By Zero",             /*  0 */
    "Debug",                        /*  1 */
    "Non Maskable Interrupt",       /*  2 */
    "Breakpoint",                   /*  3 */
    "Overflow",                     /*  4 */
    "Bound Range Exceeded",         /*  5 */
    "Invalid Opcode",               /*  6 */
    "Device Not Available",         /*  7 */
    "Double Fault",                 /*  8 */
    "Coprocessor Segment Overrun",  /*  9 */
    "Invalid TSS",                  /* 10 */
    "Segment Not Present",          /* 11 */
    "Stack Fault",                  /* 12 */
    "General Protection Fault",     /* 13 */
    "Page Fault",                   /* 14 */
    "Reserved",                     /* 15 */
    "x87 FPU Error",                /* 16 */
    "Alignment Check",              /* 17 */
    "Machine Check",                /* 18 */
    "SIMD FPU Exception",           /* 19 */
    "Virtualization Exception",     /* 20 */
    "Control Protection Exception", /* 21 */
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Security Exception", /* 30 */
    "Reserved"            /* 31 */
};

/* ------------------------------------------------------------------ */
static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;

/* ISR stubs declared in isr.asm */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

/* ------------------------------------------------------------------ */
static void idt_set_gate(uint8_t num, uint32_t base,
                         uint16_t selector, uint8_t flags)
{
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

/* ------------------------------------------------------------------ */
void idt_init(void)
{
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;

    /* Clear all entries first */
    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, 0, 0, 0);

    /* CPU exception handlers (vectors 0–31) */
    idt_set_gate(0, (uint32_t)isr0, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(1, (uint32_t)isr1, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(2, (uint32_t)isr2, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(3, (uint32_t)isr3, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(4, (uint32_t)isr4, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(5, (uint32_t)isr5, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(6, (uint32_t)isr6, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(7, (uint32_t)isr7, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(8, (uint32_t)isr8, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(9, (uint32_t)isr9, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);

    /* Hardware IRQ handlers (vectors 32–47) */
    idt_set_gate(32, (uint32_t)irq0, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(33, (uint32_t)irq1, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(34, (uint32_t)irq2, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(35, (uint32_t)irq3, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(36, (uint32_t)irq4, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(37, (uint32_t)irq5, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(38, (uint32_t)irq6, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(39, (uint32_t)irq7, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(40, (uint32_t)irq8, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(41, (uint32_t)irq9, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);

    __asm__ volatile("lidt %0" : : "m"(idtp));
}

/* ------------------------------------------------------------------ */
/* Full CPU state pushed by the ISR stubs (isr.asm)                   */
/* ------------------------------------------------------------------ */
struct registers
{
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed));

/* ------------------------------------------------------------------ */
/* Exception handler — print panic screen and halt                    */
/* ------------------------------------------------------------------ */
void isr_handler(struct registers *r)
{
    vga_set_color(VGA_WHITE, VGA_RED);
    vga_print("\n !!! KERNEL PANIC !!! \n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    vga_print("Exception #");
    vga_print_int((int32_t)r->int_no);
    vga_print(": ");
    if (r->int_no < 32)
        vga_print(exception_messages[r->int_no]);
    else
        vga_print("Unknown");

    vga_print("\nEIP=0x");
    vga_print_hex(r->eip);
    vga_print("  CS=0x");
    vga_print_hex(r->cs);
    vga_print("  ERR=0x");
    vga_print_hex(r->err_code);
    vga_print("\nEAX=0x");
    vga_print_hex(r->eax);
    vga_print("  EBX=0x");
    vga_print_hex(r->ebx);
    vga_print("  ECX=0x");
    vga_print_hex(r->ecx);
    vga_print("  EDX=0x");
    vga_print_hex(r->edx);
    vga_print("\nESP=0x");
    vga_print_hex(r->esp);
    vga_print("  EBP=0x");
    vga_print_hex(r->ebp);
    vga_print("\nSystem halted.\n");

    __asm__ volatile("cli");
    while (1)
        __asm__ volatile("hlt");
}

/* ------------------------------------------------------------------ */
/*
 * IRQ handler — timer and keyboard.
 *
 * Timer tick divider: we run the PIT at 100 Hz but only call
 * scheduler_tick() every TICK_DIVISOR ticks, giving each process a
 * 100 ms time-slice.  This keeps the shell responsive — a single
 * keystroke won't be interrupted mid-execution.
 */
#define TICK_DIVISOR 10 /* 100 Hz / 10 = 10 Hz preemption = 100 ms slice */

void irq_handler(struct registers *r)
{
    if (r->int_no == 32)
    {                    /* IRQ 0 — PIT timer */
        timer_handler(); /* increments tick counter, sends EOI */

        static uint32_t div = 0;
        if (++div >= TICK_DIVISOR)
        {
            div = 0;
            scheduler_tick(); /* preemptive round-robin switch */
        }
    }

    if (r->int_no == 33)
    {                       /* IRQ 1 — PS/2 keyboard */
        keyboard_handler(); /* reads scancode, pushes to buffer */
    }
}