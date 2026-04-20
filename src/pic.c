#include "pic.h"

/* I/O port helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Small I/O delay — write to an unused port to let the PIC settle */
static inline void io_wait(void) {
    outb(0x80, 0x00);
}

void pic_init(void) {
    /* ICW1: start initialization sequence (cascade mode, ICW4 needed) */
    outb(PIC_MASTER_CMD,  ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC_SLAVE_CMD,   ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: vector offsets */
    outb(PIC_MASTER_DATA, PIC_MASTER_OFFSET);  /* master: IRQ0-7 -> 0x20-0x27 */
    io_wait();
    outb(PIC_SLAVE_DATA,  PIC_SLAVE_OFFSET);   /* slave:  IRQ8-15 -> 0x28-0x2F */
    io_wait();

    /* ICW3: cascade wiring */
    outb(PIC_MASTER_DATA, 0x04);  /* master: slave connected to IRQ2 (bit 2) */
    io_wait();
    outb(PIC_SLAVE_DATA,  0x02);  /* slave: cascade identity = 2 */
    io_wait();

    /* ICW4: 8086 mode */
    outb(PIC_MASTER_DATA, ICW4_8086);
    io_wait();
    outb(PIC_SLAVE_DATA,  ICW4_8086);
    io_wait();

    /* Unmask all IRQs (allow all interrupts through) */
    outb(PIC_MASTER_DATA, 0x00);
    outb(PIC_SLAVE_DATA,  0x00);
}

void pic_send_eoi(uint8_t irq) {
    /* Slave PIC must also receive EOI for IRQ8-15 */
    if (irq >= 8) {
        outb(PIC_SLAVE_CMD, PIC_EOI);
    }
    outb(PIC_MASTER_CMD, PIC_EOI);
}
