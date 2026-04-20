#ifndef PMM_H
#define PMM_H

#include "types.h"
#include "multiboot.h"

#define PAGE_SIZE 4096

/* Initialise the PMM from the Multiboot memory map */
void     pmm_init(multiboot_info_t* mb);

/* Allocate one 4 KB page; returns physical address or NULL if none free */
void*    pmm_alloc_page(void);

/* Free a previously allocated page; ignores non-page-aligned addresses */
void     pmm_free_page(void* addr);

/* Return the number of free pages remaining */
uint32_t pmm_get_free_page_count(void);

#endif /* PMM_H */
