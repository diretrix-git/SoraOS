#include "paging.h"
#include "pmm.h"
#include "vga.h"

static struct page_directory *current_directory = 0;

static inline void invlpg(uint32_t addr)
{
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static inline void flush_tlb(void)
{
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
}

static inline void enable_paging(void)
{
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; /* Set PG bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

static inline void set_cr3(uint32_t addr)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(addr));
}

void paging_init(void)
{
    /* Allocate page directory */
    uint32_t pd_addr = pmm_alloc_frame();
    current_directory = (struct page_directory *)pd_addr;

    /* Zero out page directory */
    for (int i = 0; i < 1024; i++)
    {
        current_directory->tables[i] = 0;
        current_directory->tables_physical[i] = 0;
    }

    /* Identity map first 4MB for kernel */
    for (uint32_t addr = 0; addr < 0x400000; addr += PAGE_SIZE)
    {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Load CR3 with page directory physical address */
    set_cr3(pd_addr);

    /* Enable paging */
    enable_paging();

    flush_tlb();
}

void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    /* Allocate page table if not present */
    if (!current_directory->tables[pd_index])
    {
        uint32_t pt_addr = pmm_alloc_frame();
        current_directory->tables[pd_index] = (struct page_table *)pt_addr;
        current_directory->tables_physical[pd_index] = pt_addr | PAGE_PRESENT | PAGE_WRITE;

        /* Zero out page table */
        for (int i = 0; i < 1024; i++)
        {
            current_directory->tables[pd_index]->entries[i] = 0;
        }
    }

    /* Map the page */
    current_directory->tables[pd_index]->entries[pt_index] = physical_addr | flags;

    invlpg(virtual_addr);
}

void unmap_page(uint32_t virtual_addr)
{
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    if (current_directory->tables[pd_index])
    {
        current_directory->tables[pd_index]->entries[pt_index] = 0;
        invlpg(virtual_addr);
    }
}

void page_fault_handler(uint32_t error_code, uint32_t faulting_address)
{
    vga_print("\n!!! PAGE FAULT !!!\n");
    vga_print("Faulting address: 0x");
    vga_print_hex(faulting_address);
    vga_print("\nError code: 0x");
    vga_print_hex(error_code);
    vga_print("\n");

    vga_print("  Present=");
    vga_putchar((error_code & 0x1) ? '1' : '0');
    vga_print("  R/W=");
    vga_putchar((error_code & 0x2) ? 'W' : 'R');
    vga_print("  User=");
    vga_print((error_code & 0x4) ? "Yes" : "No");
    vga_print("\n");

    while (1)
    {
        __asm__ volatile("hlt");
    }
}
