#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* VGA 4-bit color codes */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW        = 14,
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

/* Core output */
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_print(const char* s);
void vga_print_color(const char* s, uint8_t color);
void vga_set_color(uint8_t fg, uint8_t bg);
uint8_t vga_get_color(void);

/* Cursor control */
void vga_set_cursor(uint32_t row, uint32_t col);
void vga_get_cursor(uint32_t* row, uint32_t* col);
void vga_enable_cursor(void);
void vga_disable_cursor(void);

/* Direct cell write (for status bar) */
void vga_write_at(uint32_t row, uint32_t col, char c, uint8_t color);
void vga_print_at(uint32_t row, uint32_t col, const char* s, uint8_t color);

/* Status bar */
void vga_draw_statusbar(void);

#endif /* VGA_H */
