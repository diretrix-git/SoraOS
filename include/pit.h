#ifndef PIT_H
#define PIT_H

#include "types.h"

/* PIT I/O ports */
#define PIT_CHANNEL0    0x40  /* Channel 0 data port */
#define PIT_CMD         0x43  /* Mode/command register */

/* PIT command: channel 0, lobyte/hibyte, mode 3 (square wave), binary */
#define PIT_CMD_CHANNEL0_MODE3  0x36

/* PIT base oscillator frequency in Hz */
#define PIT_BASE_FREQ   1193182

/* Default timer frequency */
#define PIT_DEFAULT_HZ  100

/*
 * Initialise PIT channel 0 to fire IRQ0 at frequency_hz.
 * Registers timer_handler for IRQ0 via irq_register_handler().
 */
void pit_init(uint32_t frequency_hz);

/* Return the number of timer ticks since pit_init() was called. */
uint32_t get_tick_count(void);

/*
 * Busy-wait until the tick counter has advanced by at least ticks.
 * Interrupts must be enabled for this to make progress.
 */
void sleep_ticks(uint32_t ticks);

#endif /* PIT_H */
