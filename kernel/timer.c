#include "timer.h"
#include "pic.h"

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static volatile uint32_t timer_ticks = 0;

void timer_init(uint32_t frequency)
{
    uint32_t divisor = 1193180 / frequency;

    /* Send command byte to PIT channel 0 */
    outb(0x43, 0x36);

    /* Send frequency divisor */
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    /* Unmask the timer IRQ so the scheduler tick works */
    pic_unmask(0);
}

uint32_t timer_get_ticks(void)
{
    return timer_ticks;
}

void timer_handler(void)
{
    timer_ticks++;
    pic_send_eoi(0);
}
