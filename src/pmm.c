#include "pmm.h"
#include "multiboot.h"

/* Maximum physical memory we track: 256 MB = 65536 pages */
#define PMM_MAX_PAGES   65536
#define BITMAP_SIZE     (PMM_MAX_PAGES / 32)

/* Bitmap: bit=0 free, bit=1 used */
static uint32_t pmm_bitmap[BITMAP_SIZE];
static uint32_t pmm_total_pages = 0;
static uint32_t pmm_free_pages  = 0;

/* Symbol exported by linker script — end of kernel image */
extern uint32_t kernel_end;

/* ── Bitmap helpers ─────────────────────────────────────────────────────── */

static inline void bitmap_set(uint32_t page) {
    pmm_bitmap[page / 32] |= (1u << (page % 32));
}

static inline void bitmap_clear(uint32_t page) {
    pmm_bitmap[page / 32] &= ~(1u << (page % 32));
}

static inline int bitmap_test(uint32_t page) {
    return (pmm_bitmap[page / 32] >> (page % 32)) & 1;
}

/* Minimal memset — no libc */
static void pmm_memset(void* dst, uint8_t val, size_t n) {
    uint8_t* p = (uint8_t*)dst;
    for (size_t i = 0; i < n; i++) p[i] = val;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void pmm_init(multiboot_info_t* mb) {
    /* Mark everything as used to start */
    pmm_memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));
    pmm_free_pages = 0;

    /* Determine total pages from mem_upper (KB above 1 MB) */
    if (mb->flags & MULTIBOOT_FLAG_MEM) {
        uint32_t total_kb = 1024 + mb->mem_upper; /* lower 1 MB + upper */
        pmm_total_pages = total_kb / 4;            /* 4 KB per page */
        if (pmm_total_pages > PMM_MAX_PAGES)
            pmm_total_pages = PMM_MAX_PAGES;
    } else {
        pmm_total_pages = PMM_MAX_PAGES;
    }

    /* Walk the Multiboot memory map and free available regions */
    if (mb->flags & MULTIBOOT_FLAG_MMAP) {
        multiboot_mmap_entry_t* entry =
            (multiboot_mmap_entry_t*)(uintptr_t)mb->mmap_addr;
        multiboot_mmap_entry_t* end =
            (multiboot_mmap_entry_t*)(uintptr_t)(mb->mmap_addr + mb->mmap_length);

        while (entry < end) {
            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE &&
                entry->addr_high == 0) /* only handle 32-bit addresses */
            {
                uint32_t base  = entry->addr_low;
                uint32_t len   = entry->len_low;
                uint32_t first = (base + PAGE_SIZE - 1) / PAGE_SIZE;
                uint32_t last  = (base + len) / PAGE_SIZE;

                for (uint32_t p = first; p < last && p < pmm_total_pages; p++) {
                    if (bitmap_test(p)) {
                        bitmap_clear(p);
                        pmm_free_pages++;
                    }
                }
            }
            /* Advance: entry->size does not include the size field itself */
            entry = (multiboot_mmap_entry_t*)
                    ((uintptr_t)entry + entry->size + sizeof(entry->size));
        }
    }

    /* Re-protect the kernel image (page 0 through kernel_end) */
    uint32_t kernel_pages = ((uint32_t)(uintptr_t)&kernel_end + PAGE_SIZE - 1)
                            / PAGE_SIZE;
    for (uint32_t p = 0; p < kernel_pages && p < pmm_total_pages; p++) {
        if (!bitmap_test(p)) {
            bitmap_set(p);
            if (pmm_free_pages > 0) pmm_free_pages--;
        }
    }
}

void* pmm_alloc_page(void) {
    for (uint32_t i = 0; i < pmm_total_pages / 32; i++) {
        if (pmm_bitmap[i] == 0xFFFFFFFF) continue; /* all used */
        for (uint32_t bit = 0; bit < 32; bit++) {
            uint32_t page = i * 32 + bit;
            if (!bitmap_test(page)) {
                bitmap_set(page);
                pmm_free_pages--;
                return (void*)(uintptr_t)(page * PAGE_SIZE);
            }
        }
    }
    return NULL; /* out of memory */
}

void pmm_free_page(void* addr) {
    uint32_t a = (uint32_t)(uintptr_t)addr;
    if (a % PAGE_SIZE != 0) return; /* ignore misaligned */
    uint32_t page = a / PAGE_SIZE;
    if (page >= pmm_total_pages) return;
    if (bitmap_test(page)) {
        bitmap_clear(page);
        pmm_free_pages++;
    }
}

uint32_t pmm_get_free_page_count(void) {
    return pmm_free_pages;
}
