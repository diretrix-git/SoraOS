#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

/*
 * Simple kernel heap — first-fit free-list allocator.
 * Lives at physical 2MB, grows upward to 8MB.
 * Paging must NOT be enabled before heap_init() because we use
 * physical addresses directly (identity-mapped region).
 */

void heap_init(void);
void *kmalloc(uint32_t size);
void *kmalloc_aligned(uint32_t size, uint32_t align);
void kfree(void *ptr);
uint32_t heap_used(void);
uint32_t heap_free(void);

#endif