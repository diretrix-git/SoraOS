/*
 * Stage 2 Bootloader - C Implementation
 * 
 * Responsibilities:
 * 1. Switch CPU from 16-bit real mode to 32-bit protected mode
 * 2. Load the Global Descriptor Table (GDT)
 * 3. Enable the A20 line for full 32-bit addressing
 * 4. Load the kernel from disk
 * 5. Jump to the kernel entry point
 */

#include <stdint.h>

/* =============================================================================
 * VGA Text Mode - For debugging during boot
 * ============================================================================= */
#define VGA_ADDR 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer = (uint16_t*)VGA_ADDR;
static int vga_row = 0;
static int vga_col = 0;

static void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else {
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = 0x0700 | c;
        vga_col++;
    }
    
    if (vga_row >= VGA_HEIGHT) {
        /* Simple scroll - clear screen */
        for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            vga_buffer[i] = 0x0700;
        }
        vga_row = 0;
        vga_col = 0;
    }
}

static void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

static void vga_print_hex(uint32_t value) {
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        vga_putchar(hex_chars[(value >> (i * 4)) & 0xF]);
    }
}

/* =============================================================================
 * Port I/O Helpers
 * ============================================================================= */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/* =============================================================================
 * GDT (Global Descriptor Table)
 * ============================================================================= */
#define GDT_ENTRIES 3

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdtp;

/* GDT Access byte flags */
#define GDT_ACCESS_PRESENT 0x80
#define GDT_ACCESS_RING0   0x00
#define GDT_ACCESS_RING3   0x60
#define GDT_ACCESS_SEGMENT 0x10
#define GDT_ACCESS_CODE    0x0A
#define GDT_ACCESS_DATA    0x02

/* GDT Granularity flags */
#define GDT_GRAN_4KB       0x80
#define GDT_GRAN_32BIT     0x40

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

static void gdt_init(void) {
    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base = (uint32_t)&gdt;
    
    /* Null descriptor (required by Intel spec) */
    gdt_set_gate(0, 0, 0, 0, 0);
    
    /* Kernel code segment: base=0, limit=4GB, ring 0, executable */
    gdt_set_gate(1, 0, 0xFFFFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | GDT_ACCESS_CODE, 
                 GDT_GRAN_4KB | GDT_GRAN_32BIT);
    
    /* Kernel data segment: base=0, limit=4GB, ring 0, writable */
    gdt_set_gate(2, 0, 0xFFFFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | GDT_ACCESS_DATA, 
                 GDT_GRAN_4KB | GDT_GRAN_32BIT);
    
    /* Load GDT */
    __asm__ volatile ("lgdt %0" : : "m"(gdtp));
}

/* =============================================================================
 * A20 Line Enable
 * The A20 line controls whether addresses above 1MB wrap around or not.
 * We need it enabled for protected mode.
 * ============================================================================= */
static void a20_enable(void) {
    /* Wait for keyboard controller to be ready */
    while (inb(0x64) & 0x02);
    
    /* Send command to write output port */
    outb(0x64, 0xD1);
    
    /* Wait for keyboard controller to be ready */
    while (inb(0x64) & 0x02);
    
    /* Enable A20 (set bit 1 of output port) */
    outb(0x60, 0xDF);
    
    /* Wait for keyboard controller to be ready */
    while (inb(0x64) & 0x02);
}

/* =============================================================================
 * Simple disk read in protected mode (using port I/O to IDE controller)
 * This is a simplified PIO read for the first few sectors.
 * ============================================================================= */
static void wait_ide_ready(void) {
    while (inb(0x1F7) & 0x80);  /* Wait while controller is busy */
}

static void read_sectors(uint32_t lba, uint8_t count, uint32_t buffer) {
    wait_ide_ready();
    
    /* Select drive 0, set LBA mode */
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Set sector count */
    outb(0x1F2, count);
    
    /* Set LBA address (low 24 bits) */
    outb(0x1F3, (lba >> 0) & 0xFF);
    outb(0x1F4, (lba >> 8) & 0xFF);
    outb(0x1F5, (lba >> 16) & 0xFF);
    
    /* Send READ SECTORS command */
    outb(0x1F7, 0x20);
    
    /* Read sectors */
    wait_ide_ready();
    uint16_t* buf = (uint16_t*)buffer;
    for (int s = 0; s < count; s++) {
        wait_ide_ready();
        for (int i = 0; i < 256; i++) {
            *buf++ = inw(0x1F0);
        }
    }
}

void stage2_main(void) {
    vga_print("Stage 2: Protected mode initialized\n");
    
    vga_print("Stage 2: Loading kernel...\n");
    read_sectors(5, 50, 0x100000);
    vga_print("Stage 2: Kernel loaded to 0x100000\n");
    
    uint32_t* kernel_start = (uint32_t*)0x100000;
    vga_print("Stage 2: First word: ");
    vga_print_hex(*kernel_start);
    vga_print("\n");
    
    vga_print("Stage 2: Jumping to kernel...\n");
    
    typedef void (*kernel_entry_t)(void);
    kernel_entry_t kernel_entry = (kernel_entry_t)0x100000;
    kernel_entry();
    
    vga_print("Stage 2: ERROR - Kernel returned!\n");
    while (1) {
        __asm__ volatile ("hlt");
    }
}
