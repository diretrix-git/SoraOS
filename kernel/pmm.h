#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PMM_FRAME_SIZE 4096

void pmm_init(uint32_t total_memory);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t addr);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);

#endif
