#include "gdt.h"

/* Three GDT entries: null, kernel code (0x08), kernel data (0x10) */
static gdt_entry_t gdt_entries[3];
static gdt_ptr_t   gdt_ptr;

/*
 * Fill one GDT entry.
 *
 * base   - 32-bit linear base address of the segment
 * limit  - 20-bit segment limit (in pages when granularity=1)
 * access - access byte (present, DPL, type flags)
 * gran   - upper nibble: granularity/size flags; lower nibble: limit[19:16]
 */
static void gdt_set_entry(int idx, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t gran)
{
    gdt_entries[idx].base_low    = (uint16_t)(base & 0xFFFF);
    gdt_entries[idx].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt_entries[idx].base_high   = (uint8_t)((base >> 24) & 0xFF);

    gdt_entries[idx].limit_low   = (uint16_t)(limit & 0xFFFF);
    /* Upper 4 bits of limit merged with granularity flags */
    gdt_entries[idx].granularity = (uint8_t)((gran & 0xF0) |
                                             ((limit >> 16) & 0x0F));

    gdt_entries[idx].access      = access;
}

void gdt_init(void)
{
    /* Entry 0: null descriptor (required by x86 spec) */
    gdt_set_entry(0, 0, 0, 0, 0);

    /*
     * Entry 1: kernel code segment — selector 0x08
     *   base=0, limit=0xFFFFF (4 GB with G=1)
     *   access: present(1) | DPL=0 | S=1 | type=execute/read (0xA)
     *     => 0x9A
     *   gran:   G=1 (4 KB pages) | D/B=1 (32-bit) | L=0 | AVL=0
     *     => 0xCF (upper nibble 0xC, limit[19:16]=0xF)
     */
    gdt_set_entry(1, 0x00000000, 0xFFFFF, 0x9A, 0xCF);

    /*
     * Entry 2: kernel data segment — selector 0x10
     *   base=0, limit=0xFFFFF (4 GB with G=1)
     *   access: present(1) | DPL=0 | S=1 | type=read/write (0x2)
     *     => 0x92
     *   gran:   same as code => 0xCF
     */
    gdt_set_entry(2, 0x00000000, 0xFFFFF, 0x92, 0xCF);

    /* Set up the GDT pointer (limit = size - 1) */
    gdt_ptr.limit = (uint16_t)(sizeof(gdt_entries) - 1);
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    /* Load the GDT and reload all segment registers (see gdt_flush.asm) */
    gdt_flush(&gdt_ptr);
}
