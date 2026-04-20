#ifndef IDT_H
#define IDT_H

#include "types.h"

/*
 * Register frame built on the kernel stack by the ISR stub.
 *
 * Stack layout when common_isr_handler(registers_t*) is called
 * (ESP passed as arg, so regs points to ds field):
 *
 *  [ESP+0]  ds          — saved by stub (push eax after mov ax,ds)
 *  [ESP+4]  edi         ┐
 *  [ESP+8]  esi         │
 *  [ESP+12] ebp         │  pusha pushes in this order:
 *  [ESP+16] esp_dummy   │  eax,ecx,edx,ebx,esp,ebp,esi,edi
 *  [ESP+20] ebx         │  (pusha pushes edi first at lowest addr)
 *  [ESP+24] edx         │
 *  [ESP+28] ecx         │
 *  [ESP+32] eax         ┘
 *  [ESP+36] int_no      — vector number pushed by stub
 *  [ESP+40] err_code    — real or dummy 0
 *  [ESP+44] eip         ┐
 *  [ESP+48] cs          │  pushed by CPU on interrupt
 *  [ESP+52] eflags      ┘
 */
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
} __attribute__((packed)) registers_t;

/* A single 8-byte IDT gate descriptor */
typedef struct {
    uint16_t base_low;   /* Lower 16 bits of handler address */
    uint16_t sel;        /* Kernel code segment selector (0x08) */
    uint8_t  always0;    /* Must be zero */
    uint8_t  flags;      /* Gate type, DPL, present bit */
    uint16_t base_high;  /* Upper 16 bits of handler address */
} __attribute__((packed)) idt_entry_t;

/* IDT pointer loaded via lidt */
typedef struct {
    uint16_t limit;  /* Size of IDT in bytes minus 1 */
    uint32_t base;   /* Linear address of the IDT */
} __attribute__((packed)) idt_ptr_t;

/* Initialise the IDT and load it via lidt */
void idt_init(void);

/*
 * Register a C handler for a hardware IRQ (0–15).
 * The handler is called from common_isr_handler with the full register frame.
 */
void irq_register_handler(uint8_t irq, void (*handler)(registers_t *));

/*
 * Common C handler called by every ISR stub.
 * Dispatches to registered IRQ handlers or calls kernel_panic() for
 * unhandled CPU exceptions (vectors 0–31).
 */
void common_isr_handler(registers_t *regs);

/* Forward declarations — implemented in other translation units */
void kernel_panic(const char *message);
void pic_send_eoi(uint8_t irq);

#endif /* IDT_H */
