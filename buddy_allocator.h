#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include "allocator.h"

#include <stdint.h>

IAllocator bda_create_allocator(uint8_t total_range_size_log2,
                                uint8_t min_alloc_size_log2,
                                uint8_t max_alloc_size_log2);

void bda_free_allocator(IAllocator *all);

#endif
