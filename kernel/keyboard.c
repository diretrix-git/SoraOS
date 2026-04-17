#include "keyboard.h"
#include "pic.h"

static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t keyboard_head = 0;
static volatile uint32_t keyboard_tail = 0;

static int shift_pressed = 0;
static int caps_lock = 0;

/* US QWERTY scancode to ASCII table */
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '};

static const char scancode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '};

static void keyboard_buffer_push(char c)
{
    uint32_t next_head = (keyboard_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_head != keyboard_tail)
    {
        keyboard_buffer[keyboard_head] = c;
        keyboard_head = next_head;
    }
}

static char keyboard_buffer_pop(void)
{
    if (keyboard_head == keyboard_tail)
    {
        return 0;
    }
    char c = keyboard_buffer[keyboard_tail];
    keyboard_tail = (keyboard_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

void keyboard_init(void)
{
    keyboard_head = 0;
    keyboard_tail = 0;
    shift_pressed = 0;
    caps_lock = 0;

    /* Unmask IRQ1 */
    pic_unmask(1);
}

void keyboard_handler(void)
{
    uint8_t scancode = inb(0x60);

    /* Check for key release (high bit set) */
    if (scancode & 0x80)
    {
        scancode &= 0x7F;

        if (scancode == 0x2A || scancode == 0x36)
        {
            shift_pressed = 0;
        }
    }
    else
    {
        /* Key press */
        if (scancode == 0x2A || scancode == 0x36)
        {
            shift_pressed = 1;
        }
        else if (scancode == 0x3A)
        {
            caps_lock = !caps_lock;
        }
        else if (scancode == 0x1C)
        {
            /* Enter key */
            keyboard_buffer_push('\n');
        }
        else if (scancode == 0x0E)
        {
            /* Backspace */
            keyboard_buffer_push('\b');
        }
        else if (scancode < sizeof(scancode_to_ascii))
        {
            char c;

            if (shift_pressed)
            {
                c = scancode_to_ascii_shift[scancode];
            }
            else if (caps_lock && scancode_to_ascii[scancode] >= 'a' && scancode_to_ascii[scancode] <= 'z')
            {
                c = scancode_to_ascii[scancode] - 'a' + 'A';
            }
            else
            {
                c = scancode_to_ascii[scancode];
            }

            if (c != 0)
            {
                keyboard_buffer_push(c);
            }
        }
    }

    pic_send_eoi(1);
}

char keyboard_getchar(void)
{
    char c;
    while ((c = keyboard_buffer_pop()) == 0)
    {
        __asm__ volatile("hlt");
    }
    return c;
}
