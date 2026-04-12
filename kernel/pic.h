#ifndef PIC_H
#define PIC_H

#include <stdint.h>

/* PIC Ports */
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

/* PIC Commands */
#define PIC_EOI         0x20    /* End of Interrupt */

/* ICW1 flags */
#define ICW1_ICW4       0x01    /* ICW4 present */
#define ICW1_SINGLE     0x02    /* Single mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 */
#define ICW1_LEVEL      0x08    /* Level triggered */
#define ICW1_INIT       0x10    /* Initialization */

/* ICW4 flags */
#define ICW4_8086       0x01    /* 8086 mode */
#define ICW4_AUTO       0x02    /* Auto EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode, slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode, master */
#define ICW4_SFNM       0x10    /* Special fully nested mode */

void pic_init(void);
void pic_send_eoi(uint8_t irq);
void pic_mask(uint8_t irq);
void pic_unmask(uint8_t irq);

#endif
