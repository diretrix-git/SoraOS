#include "vga.h"

#define VGA_ADDR 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer = (uint16_t*)VGA_ADDR;
static int vga_row = 0;
static int vga_col = 0;
static uint8_t vga_color = 0x07;

static uint16_t vga_make_color(uint8_t fg, uint8_t bg) {
    return (uint16_t)((bg << 4) | fg);
}

void vga_init(void) {
    vga_row = 0;
    vga_col = 0;
    vga_color = vga_make_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_color | ' ';
    }
    vga_row = 0;
    vga_col = 0;
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_color = vga_make_color(fg, bg);
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
    } else {
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_color | (uint8_t)c;
        vga_col++;
    }
    
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
    
    if (vga_row >= VGA_HEIGHT) {
        /* Scroll screen up by one row */
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
        }
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            vga_buffer[i] = vga_color | ' ';
        }
        vga_row = VGA_HEIGHT - 1;
    }
}

void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_print_hex(uint32_t value) {
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        vga_putchar(hex_chars[(value >> (i * 4)) & 0xF]);
    }
}

void vga_print_int(int32_t value) {
    if (value < 0) {
        vga_putchar('-');
        value = -value;
    }
    
    char buffer[12];
    int i = 0;
    
    if (value == 0) {
        vga_putchar('0');
        return;
    }
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (i > 0) {
        vga_putchar(buffer[--i]);
    }
}
