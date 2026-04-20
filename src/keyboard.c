#include "keyboard.h"
#include "idt.h"

#define KB_BUFFER_SIZE 256
#define KB_DATA_PORT   0x60

/* Circular input buffer */
static char    kb_buffer[KB_BUFFER_SIZE];
static uint8_t kb_head = 0;
static uint8_t kb_tail = 0;

/* Shift key state */
static uint8_t shift_held = 0;

/*
 * PS/2 Set-1 scancode → ASCII tables
 * Index = scancode (0x00–0x7F)
 * Two tables: unshifted and shifted
 */
static const char scancode_normal[128] = {
    0,    /* 0x00 — unused        */
    0,    /* 0x01 — Escape        */
    '1',  /* 0x02 */
    '2',  /* 0x03 */
    '3',  /* 0x04 */
    '4',  /* 0x05 */
    '5',  /* 0x06 */
    '6',  /* 0x07 */
    '7',  /* 0x08 */
    '8',  /* 0x09 */
    '9',  /* 0x0A */
    '0',  /* 0x0B */
    '-',  /* 0x0C */
    '=',  /* 0x0D */
    '\b', /* 0x0E — Backspace     */
    '\t', /* 0x0F — Tab           */
    'q',  /* 0x10 */
    'w',  /* 0x11 */
    'e',  /* 0x12 */
    'r',  /* 0x13 */
    't',  /* 0x14 */
    'y',  /* 0x15 */
    'u',  /* 0x16 */
    'i',  /* 0x17 */
    'o',  /* 0x18 */
    'p',  /* 0x19 */
    '[',  /* 0x1A */
    ']',  /* 0x1B */
    '\n', /* 0x1C — Enter         */
    0,    /* 0x1D — Left Ctrl     */
    'a',  /* 0x1E */
    's',  /* 0x1F */
    'd',  /* 0x20 */
    'f',  /* 0x21 */
    'g',  /* 0x22 */
    'h',  /* 0x23 */
    'j',  /* 0x24 */
    'k',  /* 0x25 */
    'l',  /* 0x26 */
    ';',  /* 0x27 */
    '\'', /* 0x28 */
    '`',  /* 0x29 */
    0,    /* 0x2A — Left Shift    */
    '\\', /* 0x2B */
    'z',  /* 0x2C */
    'x',  /* 0x2D */
    'c',  /* 0x2E */
    'v',  /* 0x2F */
    'b',  /* 0x30 */
    'n',  /* 0x31 */
    'm',  /* 0x32 */
    ',',  /* 0x33 */
    '.',  /* 0x34 */
    '/',  /* 0x35 */
    0,    /* 0x36 — Right Shift   */
    '*',  /* 0x37 — Keypad *      */
    0,    /* 0x38 — Left Alt      */
    ' ',  /* 0x39 — Space         */
    0,    /* 0x3A — Caps Lock     */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F1-F10 */
    0,    /* 0x45 — Num Lock      */
    0,    /* 0x46 — Scroll Lock   */
    '7', '8', '9', '-',            /* Keypad 7-9, -  */
    '4', '5', '6', '+',            /* Keypad 4-6, +  */
    '1', '2', '3',                 /* Keypad 1-3     */
    '0', '.',                      /* Keypad 0, .    */
    0, 0, 0,                       /* 0x54-0x56      */
    0, 0,                          /* F11, F12       */
    /* 0x59-0x7F */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0
};

static const char scancode_shifted[128] = {
    0,    /* 0x00 */
    0,    /* 0x01 — Escape */
    '!',  /* 0x02 — 1 */
    '@',  /* 0x03 — 2 */
    '#',  /* 0x04 — 3 */
    '$',  /* 0x05 — 4 */
    '%',  /* 0x06 — 5 */
    '^',  /* 0x07 — 6 */
    '&',  /* 0x08 — 7 */
    '*',  /* 0x09 — 8 */
    '(',  /* 0x0A — 9 */
    ')',  /* 0x0B — 0 */
    '_',  /* 0x0C — - */
    '+',  /* 0x0D — = */
    '\b', /* 0x0E — Backspace */
    '\t', /* 0x0F — Tab */
    'Q',  /* 0x10 */
    'W',  /* 0x11 */
    'E',  /* 0x12 */
    'R',  /* 0x13 */
    'T',  /* 0x14 */
    'Y',  /* 0x15 */
    'U',  /* 0x16 */
    'I',  /* 0x17 */
    'O',  /* 0x18 */
    'P',  /* 0x19 */
    '{',  /* 0x1A */
    '}',  /* 0x1B */
    '\n', /* 0x1C — Enter */
    0,    /* 0x1D — Left Ctrl */
    'A',  /* 0x1E */
    'S',  /* 0x1F */
    'D',  /* 0x20 */
    'F',  /* 0x21 */
    'G',  /* 0x22 */
    'H',  /* 0x23 */
    'J',  /* 0x24 */
    'K',  /* 0x25 */
    'L',  /* 0x26 */
    ':',  /* 0x27 */
    '"',  /* 0x28 */
    '~',  /* 0x29 */
    0,    /* 0x2A — Left Shift */
    '|',  /* 0x2B */
    'Z',  /* 0x2C */
    'X',  /* 0x2D */
    'C',  /* 0x2E */
    'V',  /* 0x2F */
    'B',  /* 0x30 */
    'N',  /* 0x31 */
    'M',  /* 0x32 */
    '<',  /* 0x33 */
    '>',  /* 0x34 */
    '?',  /* 0x35 */
    0,    /* 0x36 — Right Shift */
    '*',  /* 0x37 */
    0,    /* 0x38 — Left Alt */
    ' ',  /* 0x39 — Space */
    0,    /* 0x3A — Caps Lock */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F1-F10 */
    0, 0,                          /* Num Lock, Scroll Lock */
    '7', '8', '9', '-',
    '4', '5', '6', '+',
    '1', '2', '3',
    '0', '.',
    0, 0, 0, 0, 0,
    /* 0x59-0x7F */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0
};

/* I/O port read */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void keyboard_handler(registers_t* regs) {
    (void)regs;
    uint8_t scancode = inb(KB_DATA_PORT);

    /* Shift press: 0x2A (left), 0x36 (right) */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_held = 1;
        return;
    }
    /* Shift release: 0xAA (left), 0xB6 (right) */
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_held = 0;
        return;
    }

    /* Ignore all other break codes (bit 7 set) */
    if (scancode & 0x80) return;

    /* Translate scancode → ASCII using correct table */
    char c = shift_held ? scancode_shifted[scancode & 0x7F]
                        : scancode_normal[scancode & 0x7F];
    if (!c) return; /* unmapped key */

    /* Push to circular buffer if not full */
    uint8_t next_head = (uint8_t)((kb_head + 1) % KB_BUFFER_SIZE);
    if (next_head != kb_tail) {
        kb_buffer[kb_head] = c;
        kb_head = next_head;
    }
    /* else: buffer full — discard */
}

void keyboard_init(void) {
    kb_head    = 0;
    kb_tail    = 0;
    shift_held = 0;
    irq_register_handler(1, keyboard_handler);
}

char keyboard_getchar(void) {
    if (kb_head == kb_tail) return 0; /* empty */
    char c = kb_buffer[kb_tail];
    kb_tail = (uint8_t)((kb_tail + 1) % KB_BUFFER_SIZE);
    return c;
}
