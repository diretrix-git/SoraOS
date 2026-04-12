#include "gdt.h"

#define GDT_ENTRIES 3

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdtp;

/* GDT Access byte flags */
#define GDT_ACCESS_PRESENT 0x80
#define GDT_ACCESS_RING0   0x00
#define GDT_ACCESS_SEGMENT 0x10
#define GDT_ACCESS_CODE    0x0A
#define GDT_ACCESS_DATA    0x02

/* GDT Granularity flags */
#define GDT_GRAN_4KB       0x80
#define GDT_GRAN_32BIT     0x40

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_init(void) {
    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base = (uint32_t)&gdt;
    
    /* Null descriptor (required by Intel spec) */
    gdt_set_gate(0, 0, 0, 0, 0);
    
    /* Kernel code segment: base=0, limit=4GB, ring 0, executable */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | GDT_ACCESS_CODE, 
                 GDT_GRAN_4KB | GDT_GRAN_32BIT);
    
    /* Kernel data segment: base=0, limit=4GB, ring 0, writable */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | GDT_ACCESS_DATA, 
                 GDT_GRAN_4KB | GDT_GRAN_32BIT);
    
    /* Load GDT */
    __asm__ volatile ("lgdt %0" : : "m"(gdtp));
    
    /* Reload segment registers with far jump */
    __asm__ volatile (
        "mov $0x08, %%eax\n"
        "push %%eax\n"
        "push $1f\n"
        "lret\n"
        "1:\n"
        "mov $0x10, %%eax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        :
        :
        : "eax"
    );
}
