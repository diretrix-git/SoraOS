#include "serial.h"
#include "types.h"

#define COM1_BASE 0x3F8

/* Flag checked by kernel_panic() before calling serial_print() */
int serial_ready = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void) {
    outb(COM1_BASE + 1, 0x00); /* Disable all interrupts */
    outb(COM1_BASE + 3, 0x80); /* Enable DLAB (set baud rate divisor) */
    outb(COM1_BASE + 0, 0x0C); /* Divisor low byte: 12 → 115200/12 = 9600 baud */
    outb(COM1_BASE + 1, 0x00); /* Divisor high byte */
    outb(COM1_BASE + 3, 0x03); /* 8 bits, no parity, 1 stop bit */
    outb(COM1_BASE + 2, 0xC7); /* Enable FIFO, clear, 14-byte threshold */
    outb(COM1_BASE + 4, 0x0B); /* IRQs enabled, RTS/DSR set */
    serial_ready = 1;
}

void serial_putchar(char c) {
    /* Wait until transmit holding register is empty (bit 5 of LSR) */
    while ((inb(COM1_BASE + 5) & 0x20) == 0)
        ;
    outb(COM1_BASE, (uint8_t)c);
}

void serial_print(const char* s) {
    while (*s)
        serial_putchar(*s++);
}
