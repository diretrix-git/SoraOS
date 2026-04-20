#ifndef PIC_H
#define PIC_H

#include "types.h"

/* 8259A PIC I/O ports */
#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD   0xA0
#define PIC_SLAVE_DATA  0xA1

/* PIC commands */
#define PIC_EOI         0x20  /* End-of-interrupt command */

/* ICW1 flags */
#define ICW1_INIT       0x10  /* Initialization */
#define ICW1_ICW4       0x01  /* ICW4 needed */

/* ICW4 flags */
#define ICW4_8086       0x01  /* 8086/88 mode */

/* IRQ vector offsets after remapping */
#define PIC_MASTER_OFFSET 0x20  /* IRQ0-7  -> vectors 0x20-0x27 */
#define PIC_SLAVE_OFFSET  0x28  /* IRQ8-15 -> vectors 0x28-0x2F */

/* Initialize and remap both PICs */
void pic_init(void);

/* Send End-of-Interrupt for the given IRQ (0-15) */
void pic_send_eoi(uint8_t irq);

#endif /* PIC_H */
