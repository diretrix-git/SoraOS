#include "idt.h"
#include "types.h"

/* -----------------------------------------------------------------------
 * IDT storage
 * --------------------------------------------------------------------- */

#define IDT_ENTRIES 256

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

/* -----------------------------------------------------------------------
 * IRQ handler table (vectors 32–47 → IRQ 0–15)
 * --------------------------------------------------------------------- */

#define IRQ_COUNT 16

static void (*irq_handlers[IRQ_COUNT])(registers_t *);

/* -----------------------------------------------------------------------
 * Exception name table (vectors 0–31)
 * --------------------------------------------------------------------- */

static const char *exception_names[32] = {
    "Division Error",                   /* 0  */
    "Debug",                            /* 1  */
    "Non-Maskable Interrupt",           /* 2  */
    "Breakpoint",                       /* 3  */
    "Overflow",                         /* 4  */
    "Bound Range Exceeded",             /* 5  */
    "Invalid Opcode",                   /* 6  */
    "Device Not Available",             /* 7  */
    "Double Fault",                     /* 8  */
    "Coprocessor Segment Overrun",      /* 9  */
    "Invalid TSS",                      /* 10 */
    "Segment Not Present",              /* 11 */
    "Stack-Segment Fault",              /* 12 */
    "General Protection Fault",         /* 13 */
    "Page Fault",                       /* 14 */
    "Reserved",                         /* 15 */
    "x87 Floating-Point Exception",     /* 16 */
    "Alignment Check",                  /* 17 */
    "Machine Check",                    /* 18 */
    "SIMD Floating-Point Exception",    /* 19 */
    "Virtualization Exception",         /* 20 */
    "Control Protection Exception",     /* 21 */
    "Reserved",                         /* 22 */
    "Reserved",                         /* 23 */
    "Reserved",                         /* 24 */
    "Reserved",                         /* 25 */
    "Reserved",                         /* 26 */
    "Reserved",                         /* 27 */
    "Hypervisor Injection Exception",   /* 28 */
    "VMM Communication Exception",      /* 29 */
    "Security Exception",               /* 30 */
    "Reserved"                          /* 31 */
};

/* -----------------------------------------------------------------------
 * External ASM ISR stubs (defined in src/isr.asm)
 * --------------------------------------------------------------------- */

/* CPU exceptions (vectors 0–31) */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

/* Hardware IRQs (vectors 32–47) */
extern void isr32(void);
extern void isr33(void);
extern void isr34(void);
extern void isr35(void);
extern void isr36(void);
extern void isr37(void);
extern void isr38(void);
extern void isr39(void);
extern void isr40(void);
extern void isr41(void);
extern void isr42(void);
extern void isr43(void);
extern void isr44(void);
extern void isr45(void);
extern void isr46(void);
extern void isr47(void);

/* -----------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------- */

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].base_low  = (uint16_t)(base & 0xFFFF);
    idt[num].base_high = (uint16_t)((base >> 16) & 0xFFFF);
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

/* -----------------------------------------------------------------------
 * idt_init — fill all 256 IDT entries and load the IDT
 * --------------------------------------------------------------------- */

void idt_init(void)
{
    /* Zero the handler table */
    for (int i = 0; i < IRQ_COUNT; i++) {
        irq_handlers[i] = (void *)0;
    }

    idt_ptr.limit = (uint16_t)(sizeof(idt_entry_t) * IDT_ENTRIES - 1);
    idt_ptr.base  = (uint32_t)&idt;

    /* 0x8E = present | ring-0 | 32-bit interrupt gate */
    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    /* Hardware IRQs (remapped to vectors 32–47 by PIC) */
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)isr34, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)isr35, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)isr36, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)isr37, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)isr38, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)isr39, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)isr40, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)isr41, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)isr42, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)isr43, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)isr44, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)isr45, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)isr46, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)isr47, 0x08, 0x8E);

    /* Remaining entries (48–255) are left as zero — not present */

    /* Load the IDT */
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

/* -----------------------------------------------------------------------
 * irq_register_handler — register a C handler for IRQ 0–15
 * --------------------------------------------------------------------- */

void irq_register_handler(uint8_t irq, void (*handler)(registers_t *))
{
    if (irq < IRQ_COUNT) {
        irq_handlers[irq] = handler;
    }
}

/* -----------------------------------------------------------------------
 * common_isr_handler — central C dispatch point for all interrupts
 * --------------------------------------------------------------------- */

void common_isr_handler(registers_t *regs)
{
    uint32_t vec = regs->int_no;

    if (vec < 32) {
        /* CPU exception — always panic (no user-registered handlers for exceptions) */
        kernel_panic(exception_names[vec]);
    } else if (vec < 48) {
        /* Hardware IRQ: vector 32 = IRQ0, vector 47 = IRQ15 */
        uint8_t irq = (uint8_t)(vec - 32);

        if (irq_handlers[irq] != (void *)0) {
            irq_handlers[irq](regs);
        }
        /* else: spurious IRQ — silently ignore */

        /* Send EOI to PIC(s) */
        pic_send_eoi(irq);
    }
    /* Vectors 48–255: not used — silently ignore */
}
