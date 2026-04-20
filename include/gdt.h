#ifndef GDT_H
#define GDT_H

#include "types.h"

/* A single GDT segment descriptor (8 bytes, packed) */
typedef struct {
    uint16_t limit_low;    /* Lower 16 bits of segment limit */
    uint16_t base_low;     /* Lower 16 bits of base address */
    uint8_t  base_middle;  /* Bits 16-23 of base address */
    uint8_t  access;       /* Access flags (type, DPL, present) */
    uint8_t  granularity;  /* Granularity + upper 4 bits of limit */
    uint8_t  base_high;    /* Bits 24-31 of base address */
} __attribute__((packed)) gdt_entry_t;

/* GDT pointer loaded via lgdt (6 bytes, packed) */
typedef struct {
    uint16_t limit;  /* Size of GDT in bytes minus 1 */
    uint32_t base;   /* Linear address of the GDT */
} __attribute__((packed)) gdt_ptr_t;

/* Initialise the GDT and reload segment registers */
void gdt_init(void);

/* Defined in gdt_flush.asm — loads the GDT and reloads CS/DS/etc. */
extern void gdt_flush(gdt_ptr_t *gdt_ptr);

#endif /* GDT_H */
