#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x01
#define PAGE_WRITE   0x02
#define PAGE_USER    0x04

struct page_table {
    uint32_t entries[1024];
};

struct page_directory {
    struct page_table* tables[1024];
    uint32_t tables_physical[1024];
};

void paging_init(void);
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page(uint32_t virtual_addr);
void page_fault_handler(uint32_t error_code, uint32_t faulting_address);

#endif
