#include "ps2.h"

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void ps2_wait_write(void)
{
    /* Wait until the input buffer is empty (bit 1 of status register) */
    int timeout = 100000;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) && --timeout)
        ;
}

void ps2_wait_read(void)
{
    /* Wait until the output buffer is full (bit 0 of status register) */
    int timeout = 100000;
    while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) && --timeout)
        ;
}

uint8_t ps2_read_data(void)
{
    ps2_wait_read();
    return inb(PS2_DATA_PORT);
}

void ps2_write_command(uint8_t cmd)
{
    ps2_wait_write();
    outb(PS2_COMMAND_PORT, cmd);
}

void ps2_write_data(uint8_t data)
{
    ps2_wait_write();
    outb(PS2_DATA_PORT, data);
}

void ps2_controller_init(void)
{
    uint8_t config;
    int timeout;

    /* Flush the output buffer with timeout */
    timeout = 100000;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) && --timeout)
        inb(PS2_DATA_PORT);

    /* Read current configuration byte */
    ps2_write_command(PS2_CMD_READ_CONFIG);
    config = ps2_read_data();

    /* Enable interrupts for port 1 */
    config |= PS2_CONFIG_PORT1_INT;
    config &= ~PS2_CONFIG_PORT2_INT;

    /* Write back configuration */
    ps2_write_command(PS2_CMD_WRITE_CONFIG);
    ps2_write_data(config);

    /* Enable port 1 */
    ps2_write_command(PS2_CMD_ENABLE_PORT1);

    /* Flush any pending data with timeout */
    timeout = 100000;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) && --timeout)
        inb(PS2_DATA_PORT);
}
