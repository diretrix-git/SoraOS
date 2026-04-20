#include "pit.h"
#include "idt.h"

/* Forward declaration — implemented in scheduler.c */
void schedule(void);

/* Global tick counter incremented on every IRQ0 */
static volatile uint32_t tick_count = 0;

/* Write a byte to an I/O port */
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* IRQ0 handler: increment tick counter and invoke the scheduler */
static void timer_handler(registers_t *regs)
{
    (void)regs;
    tick_count++;
    schedule();
}

void pit_init(uint32_t frequency_hz)
{
    uint16_t divisor = (uint16_t)(PIT_BASE_FREQ / frequency_hz);

    /* Channel 0, lobyte/hibyte access, mode 3 (square wave), binary */
    outb(PIT_CMD, PIT_CMD_CHANNEL0_MODE3);

    /* Send divisor low byte then high byte */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    /* Register our handler for IRQ0 */
    irq_register_handler(0, timer_handler);
}

uint32_t get_tick_count(void)
{
    return tick_count;
}

void sleep_ticks(uint32_t ticks)
{
    uint32_t start = tick_count;
    while ((tick_count - start) < ticks) {
        __asm__ volatile("hlt");
    }
}
