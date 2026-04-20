#include "kernel.h"
#include "vga.h"
#include "serial.h"

/* Defined in serial.c — set to 1 after serial_init() completes */
extern int serial_ready;

void kernel_panic(const char* message) {
    /* Disable interrupts immediately */
    __asm__ volatile("cli");

    /* Print to VGA: white text on red background (attribute 0x4F) */
    vga_print_color("\n*** KERNEL PANIC: ", 0x4F);
    vga_print_color(message, 0x4F);
    vga_print_color(" ***\n", 0x4F);

    /* Mirror to serial if available */
    if (serial_ready) {
        serial_print("\n*** KERNEL PANIC: ");
        serial_print(message);
        serial_print(" ***\n");
    }

    /* Halt forever */
    for (;;) {
        __asm__ volatile("hlt");
    }
}
