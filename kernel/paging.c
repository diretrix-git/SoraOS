#include "paging.h"
#include "pmm.h"
#include "vga.h"

/*
 * Paging — identity-map the first 16 MB so that virtual == physical
 * for all kernel memory.  This keeps everything simple while still
 * enabling the paging hardware.
 *
 * Memory layout after paging_init():
 *   0x000000 – 0xFFFFFF  (16 MB)  →  identity-mapped, RW, supervisor
 *
 * Page directory and page tables are allocated from the PMM.
 * All structures live inside the identity-mapped region, so we can
 * use physical addresses as pointers directly.
 */

/* ------------------------------------------------------------------ */
/* Inline helpers                                                      */
/* ------------------------------------------------------------------ */
static inline void invlpg(uint32_t addr)
{
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static inline void set_cr3(uint32_t phys_addr)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(phys_addr) : "memory");
}

static inline void enable_paging_cr0(void)
{
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000UL; /* set PG bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

/* ------------------------------------------------------------------ */
/* Module-private state                                                */
/* ------------------------------------------------------------------ */
static uint32_t *kernel_pd = 0; /* page directory (physical addr) */

/* ------------------------------------------------------------------ */
void paging_init(void)
{
    /*
     * Allocate the page directory (1 PMM frame = 4 KB = 1024 entries).
     * Each entry covers 4 MB.
     */
    uint32_t pd_phys = pmm_alloc_frame();
    if (!pd_phys)
    {
        vga_print("[PAGING] ERROR: no frame for page directory\n");
        return;
    }
    kernel_pd = (uint32_t *)pd_phys;

    /* Zero the entire directory */
    for (int i = 0; i < 1024; i++)
        kernel_pd[i] = 0;

    /*
     * Identity-map 0 – 16 MB.
     * 16 MB / 4 MB per PDE = 4 page tables required.
     * Each page table covers 1024 × 4 KB = 4 MB.
     */
    for (int t = 0; t < 4; t++)
    {
        uint32_t pt_phys = pmm_alloc_frame();
        if (!pt_phys)
        {
            vga_print("[PAGING] ERROR: no frame for page table\n");
            return;
        }
        uint32_t *pt = (uint32_t *)pt_phys;

        for (int e = 0; e < 1024; e++)
        {
            uint32_t phys = ((uint32_t)t * 1024u + (uint32_t)e) * PAGE_SIZE;
            pt[e] = phys | PAGE_PRESENT | PAGE_WRITE;
        }

        /* Write PDE: physical address of PT + flags */
        kernel_pd[t] = pt_phys | PAGE_PRESENT | PAGE_WRITE;
    }

    /* Load CR3 and enable paging */
    set_cr3(pd_phys);
    enable_paging_cr0();
}

/* ------------------------------------------------------------------ */
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    if (!kernel_pd)
        return;

    uint32_t pd_idx = (virtual_addr >> 22) & 0x3FFu;
    uint32_t pt_idx = (virtual_addr >> 12) & 0x3FFu;

    /* Allocate a page table for this directory entry if missing */
    if (!(kernel_pd[pd_idx] & PAGE_PRESENT))
    {
        uint32_t pt_phys = pmm_alloc_frame();
        if (!pt_phys)
            return;

        uint32_t *pt = (uint32_t *)pt_phys;
        for (int i = 0; i < 1024; i++)
            pt[i] = 0;

        kernel_pd[pd_idx] = pt_phys | PAGE_PRESENT | PAGE_WRITE;
    }

    uint32_t pt_phys = kernel_pd[pd_idx] & ~0xFFFu;
    uint32_t *pt = (uint32_t *)pt_phys;

    pt[pt_idx] = (physical_addr & ~0xFFFu) | (flags & 0xFFFu);
    invlpg(virtual_addr);
}

/* ------------------------------------------------------------------ */
void unmap_page(uint32_t virtual_addr)
{
    if (!kernel_pd)
        return;

    uint32_t pd_idx = (virtual_addr >> 22) & 0x3FFu;
    uint32_t pt_idx = (virtual_addr >> 12) & 0x3FFu;

    if (!(kernel_pd[pd_idx] & PAGE_PRESENT))
        return;

    uint32_t pt_phys = kernel_pd[pd_idx] & ~0xFFFu;
    uint32_t *pt = (uint32_t *)pt_phys;

    pt[pt_idx] = 0;
    invlpg(virtual_addr);
}

/* ------------------------------------------------------------------ */
void page_fault_handler(uint32_t error_code, uint32_t faulting_address)
{
    vga_print("\n!!! PAGE FAULT !!!\n");
    vga_print("Faulting address: 0x");
    vga_print_hex(faulting_address);
    vga_print("\nError code: 0x");
    vga_print_hex(error_code);
    vga_print("\nFlags: present=");
    vga_putchar((error_code & 0x1) ? '1' : '0');
    vga_print(" write=");
    vga_putchar((error_code & 0x2) ? '1' : '0');
    vga_print(" user=");
    vga_putchar((error_code & 0x4) ? '1' : '0');
    vga_print("\n");

    while (1)
        __asm__ volatile("hlt");
}