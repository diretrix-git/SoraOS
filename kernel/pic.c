#include "pic.h"

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void pic_init(void) {
    uint8_t mask1, mask2;
    
    /* Save current masks */
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);
    
    /* Start initialization sequence */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    /* ICW2: Set vector offsets */
    outb(PIC1_DATA, 0x20);    /* IRQ0-7 -> IDT 32-39 */
    outb(PIC2_DATA, 0x28);    /* IRQ8-15 -> IDT 40-47 */
    
    /* ICW3: Tell Master PIC about Slave PIC */
    outb(PIC1_DATA, 0x04);    /* IRQ2 connected to slave */
    outb(PIC2_DATA, 0x02);    /* Slave PIC cascade identity */
    
    /* ICW4: Set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    
    /* Restore masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_unmask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}
