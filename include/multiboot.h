#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "types.h"

/* Multiboot magic values */
#define MULTIBOOT_MAGIC         0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/* Multiboot header flags */
#define MULTIBOOT_FLAG_MEM      0x001   /* mem_lower/mem_upper valid */
#define MULTIBOOT_FLAG_DEVICE   0x002   /* boot_device valid */
#define MULTIBOOT_FLAG_CMDLINE  0x004   /* cmdline valid */
#define MULTIBOOT_FLAG_MODS     0x008   /* modules valid */
#define MULTIBOOT_FLAG_MMAP     0x040   /* mmap_* fields valid */

/* Memory map entry types */
#define MULTIBOOT_MEMORY_AVAILABLE  1
#define MULTIBOOT_MEMORY_RESERVED   2

/* Multiboot memory map entry */
typedef struct multiboot_mmap_entry {
    uint32_t size;          /* size of this entry (not including this field) */
    uint32_t addr_low;      /* base address (low 32 bits) */
    uint32_t addr_high;     /* base address (high 32 bits) */
    uint32_t len_low;       /* length in bytes (low 32 bits) */
    uint32_t len_high;      /* length in bytes (high 32 bits) */
    uint32_t type;          /* MULTIBOOT_MEMORY_* */
} __attribute__((packed)) multiboot_mmap_entry_t;

/* Multiboot module entry */
typedef struct multiboot_mod_entry {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
} __attribute__((packed)) multiboot_mod_entry_t;

/* Multiboot info structure passed by GRUB in EBX */
typedef struct multiboot_info {
    uint32_t flags;         /* which fields below are valid */

    /* Memory info (flags bit 0) */
    uint32_t mem_lower;     /* KB of lower memory (below 1 MB) */
    uint32_t mem_upper;     /* KB of upper memory (above 1 MB) */

    /* Boot device (flags bit 1) */
    uint32_t boot_device;

    /* Command line (flags bit 2) */
    uint32_t cmdline;

    /* Modules (flags bit 3) */
    uint32_t mods_count;
    uint32_t mods_addr;

    /* Symbol table / ELF section header (flags bits 4-5) */
    uint32_t syms[4];

    /* Memory map (flags bit 6) */
    uint32_t mmap_length;   /* total size of mmap buffer in bytes */
    uint32_t mmap_addr;     /* physical address of first mmap entry */

    /* Drives (flags bit 7) */
    uint32_t drives_length;
    uint32_t drives_addr;

    /* ROM config table (flags bit 8) */
    uint32_t config_table;

    /* Boot loader name (flags bit 9) */
    uint32_t boot_loader_name;

    /* APM table (flags bit 10) */
    uint32_t apm_table;

    /* VBE info (flags bits 11-12) */
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} __attribute__((packed)) multiboot_info_t;

#endif /* MULTIBOOT_H */
