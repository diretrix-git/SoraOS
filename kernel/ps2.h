#ifndef PS2_H
#define PS2_H

#include <stdint.h>

/* PS/2 Controller I/O ports */
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64
#define PS2_DATA_PORT 0x60

/* PS/2 Controller commands */
#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_DISABLE_PORT2 0xA7
#define PS2_CMD_ENABLE_PORT2 0xA8
#define PS2_CMD_DISABLE_PORT1 0xAD
#define PS2_CMD_ENABLE_PORT1 0xAE
#define PS2_CMD_RESET_CONTROLLER 0xFF
#define PS2_CMD_SELF_TEST 0xAA

/* PS/2 Configuration byte flags */
#define PS2_CONFIG_PORT1_INT 0x01
#define PS2_CONFIG_PORT2_INT 0x02
#define PS2_CONFIG_SYS_FLAG 0x04
#define PS2_CONFIG_PORT1_CLOCK 0x10
#define PS2_CONFIG_PORT2_CLOCK 0x20

/* PS/2 Status register flags */
#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_STATUS_INPUT_FULL 0x02
#define PS2_STATUS_SYSTEM_FLAG 0x04
#define PS2_STATUS_COMMAND_DATA 0x08
#define PS2_STATUS_PORT2_DATA 0x20
#define PS2_STATUS_TIMEOUT_ERROR 0x40
#define PS2_STATUS_PARITY_ERROR 0x80

void ps2_controller_init(void);
void ps2_wait_write(void);
void ps2_wait_read(void);
uint8_t ps2_read_data(void);
void ps2_write_command(uint8_t cmd);
void ps2_write_data(uint8_t data);

#endif
