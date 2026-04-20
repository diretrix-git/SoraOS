#include "heap.h"

/*
 * First-fit free-list heap allocator.
 *
 * Layout in memory:
 *   [block_header_t | payload bytes] [block_header_t | payload bytes] ...
 *
 * The heap region starts at HEAP_START (2 MB) and can grow up to HEAP_MAX
 * (8 MB).  Because paging identity-maps 0–16 MB, virtual == physical here.
 *
 * No locks: single-threaded kernel, interrupts don't call kmalloc.
 */

#define HEAP_MAGIC 0xC0FFEEUL
#define HEAP_START 0x200000UL  /* 2 MB  — above kernel image */
#define HEAP_MAX 0x800000UL    /* 8 MB  — upper limit        */
#define HEAP_INITIAL 0x10000UL /* 64 KB initially committed  */

typedef struct block_hdr
{
    uint32_t magic; /* sanity sentinel                  */
    uint32_t size;  /* payload size (NOT inc. header)   */
    uint8_t free;   /* 1 = available, 0 = allocated     */
    uint8_t _pad[3];
    struct block_hdr *next; /* next block in list (or NULL)     */
} block_hdr_t;

/* Current high-water mark of committed heap space */
static uint32_t heap_top = 0;
static block_hdr_t *heap_head = 0;

/* ------------------------------------------------------------------ */
/* Internal: extend the heap by at least `bytes` bytes.               */
/* ------------------------------------------------------------------ */
static void heap_extend(uint32_t bytes)
{
    /* Round up to 4 KB page boundary */
    uint32_t pages = (bytes + 4095u) / 4096u;
    uint32_t add = pages * 4096u;

    if (heap_top + add > HEAP_MAX)
        add = HEAP_MAX - heap_top; /* clamp */

    if (add == 0)
        return;

    /* The memory already exists (identity-mapped by paging_init).
     * We just advance our water-mark and let the last block absorb it. */
    heap_top += add;

    /* Grow the last free block, or append a new free block */
    block_hdr_t *cur = heap_head;
    block_hdr_t *last = 0;
    while (cur)
    {
        last = cur;
        cur = cur->next;
    }

    if (last && last->free)
    {
        last->size += add;
    }
    else
    {
        /* Append new block right after 'last' payload */
        block_hdr_t *nb;
        if (last)
            nb = (block_hdr_t *)((uint8_t *)last + sizeof(block_hdr_t) + last->size);
        else
            nb = (block_hdr_t *)HEAP_START;

        nb->magic = HEAP_MAGIC;
        nb->size = add - sizeof(block_hdr_t);
        nb->free = 1;
        nb->next = 0;
        if (last)
            last->next = nb;
        else
            heap_head = nb;
    }
}

/* ------------------------------------------------------------------ */
void heap_init(void)
{
    heap_top = HEAP_START;
    heap_head = 0;
    heap_extend(HEAP_INITIAL);
}

/* ------------------------------------------------------------------ */
void *kmalloc(uint32_t size)
{
    return kmalloc_aligned(size, 8);
}

/* ------------------------------------------------------------------ */
void *kmalloc_aligned(uint32_t size, uint32_t align)
{
    if (size == 0)
        return 0;

    /* Round up size to alignment */
    if (align < 8)
        align = 8;
    size = (size + align - 1u) & ~(align - 1u);

    /* First-fit search */
    block_hdr_t *cur = heap_head;
    while (cur)
    {
        if (cur->free && cur->size >= size)
        {
            /* Split if the leftover is large enough to be its own block */
            uint32_t leftover = cur->size - size;
            if (leftover > sizeof(block_hdr_t) + align)
            {
                block_hdr_t *split = (block_hdr_t *)((uint8_t *)cur + sizeof(block_hdr_t) + size);
                split->magic = HEAP_MAGIC;
                split->size = leftover - sizeof(block_hdr_t);
                split->free = 1;
                split->next = cur->next;
                cur->next = split;
                cur->size = size;
            }
            cur->free = 0;
            return (void *)((uint8_t *)cur + sizeof(block_hdr_t));
        }
        cur = cur->next;
    }

    /* Need more space — extend and retry once */
    uint32_t need = size + sizeof(block_hdr_t);
    if (need < 4096u)
        need = 4096u;
    heap_extend(need);

    cur = heap_head;
    while (cur)
    {
        if (cur->free && cur->size >= size)
        {
            uint32_t leftover = cur->size - size;
            if (leftover > sizeof(block_hdr_t) + align)
            {
                block_hdr_t *split = (block_hdr_t *)((uint8_t *)cur + sizeof(block_hdr_t) + size);
                split->magic = HEAP_MAGIC;
                split->size = leftover - sizeof(block_hdr_t);
                split->free = 1;
                split->next = cur->next;
                cur->next = split;
                cur->size = size;
            }
            cur->free = 0;
            return (void *)((uint8_t *)cur + sizeof(block_hdr_t));
        }
        cur = cur->next;
    }

    return 0; /* out of memory */
}

/* ------------------------------------------------------------------ */
void kfree(void *ptr)
{
    if (!ptr)
        return;

    block_hdr_t *hdr = (block_hdr_t *)((uint8_t *)ptr - sizeof(block_hdr_t));
    if (hdr->magic != HEAP_MAGIC)
        return; /* corrupt / double-free */
    if (hdr->free)
        return; /* already free          */

    hdr->free = 1;

    /* Coalesce with consecutive free blocks */
    while (hdr->next && hdr->next->free)
    {
        hdr->size += sizeof(block_hdr_t) + hdr->next->size;
        hdr->next = hdr->next->next;
    }
}

/* ------------------------------------------------------------------ */
uint32_t heap_used(void)
{
    uint32_t used = 0;
    block_hdr_t *cur = heap_head;
    while (cur)
    {
        if (!cur->free)
            used += cur->size;
        cur = cur->next;
    }
    return used;
}

uint32_t heap_free(void)
{
    uint32_t free_bytes = 0;
    block_hdr_t *cur = heap_head;
    while (cur)
    {
        if (cur->free)
            free_bytes += cur->size;
        cur = cur->next;
    }
    return free_bytes;
}