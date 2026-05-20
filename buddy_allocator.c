#include "buddy_allocator.h"
#include "allocator.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t FREE_SLOT_STATE = 0;
static const uint8_t BUSY_SLOT_STATE = 1;
static const uint8_t NOT_USED_SLOT_STATE = 2;

typedef struct {
  uint8_t state;
  uint8_t pool_index;
} SlotInfo;

typedef struct {
  uint32_t next;
  uint32_t prev;
} SlotLink;

typedef struct {
  uint32_t head;
  uint32_t tail;
} PoolInfo;

typedef struct {
  uint8_t total_range_log2;
  uint8_t min_alloc_size_log2;
  uint8_t max_alloc_size_log2;
  uint32_t pool_count;
  uint32_t slots_count;
  uint8_t *start;
  SlotInfo *slots;
  SlotLink *links;
  PoolInfo *pools;
} BuddyCtx;

BuddyCtx *bda_create_ctx(uint8_t total_range_size_log2,
                         uint8_t min_alloc_size_log2,
                         uint8_t max_alloc_size_log2) {
  if (!(total_range_size_log2 < 64 &&
        min_alloc_size_log2 < max_alloc_size_log2 &&
        max_alloc_size_log2 <= total_range_size_log2))
    return NULL;

  BuddyCtx *ctx = (BuddyCtx *)malloc(sizeof(BuddyCtx));

  if (ctx == NULL)
    return NULL;

  ctx->pool_count = 1 + max_alloc_size_log2 - min_alloc_size_log2;
  ctx->slots_count = 1 << (total_range_size_log2 - min_alloc_size_log2);
  ctx->max_alloc_size_log2 = max_alloc_size_log2;
  ctx->min_alloc_size_log2 = min_alloc_size_log2;
  ctx->total_range_log2 = total_range_size_log2;

  ctx->start = (uint8_t *)malloc(1 << total_range_size_log2);

  if (ctx->start == NULL) {
    free(ctx);
    return NULL;
  }

  uint32_t slots_per_max = 1 << (max_alloc_size_log2 - min_alloc_size_log2);

  ctx->pools = (PoolInfo *)malloc(ctx->pool_count * sizeof(PoolInfo));

  if (ctx->pools == NULL) {
    free(ctx);
    free(ctx->start);
    return NULL;
  }

  for (uint16_t i = 0; i < ctx->pool_count - 1; ++i) {
    ctx->pools[i].head = UINT32_MAX;
    ctx->pools[i].tail = UINT32_MAX;
  }
  ctx->pools[ctx->pool_count - 1].head = 0;
  ctx->pools[ctx->pool_count - 1].tail = ctx->slots_count - slots_per_max;

  ctx->links = (SlotLink *)malloc(ctx->slots_count * sizeof(SlotLink));

  if (ctx->links == NULL) {
    free(ctx);
    free(ctx->start);
    free(ctx->pools);
    return NULL;
  }

  ctx->slots = (SlotInfo *)malloc(ctx->slots_count * sizeof(SlotInfo));

  if (ctx->slots == NULL) {
    free(ctx);
    free(ctx->start);
    free(ctx->pools);
    free(ctx->links);
    return NULL;
  }

  for (uint32_t i = 0; i < ctx->slots_count; i++) {
    ctx->slots[i].state = NOT_USED_SLOT_STATE;
    ctx->slots[i].pool_index = UINT8_MAX;
  }

  for (uint32_t slot = 0; slot < ctx->slots_count; slot += slots_per_max) {
    ctx->links[slot].next = slot + slots_per_max;
    ctx->links[slot].prev = slot - slots_per_max;
    ctx->slots[slot].pool_index = ctx->pool_count - 1;
    ctx->slots[slot].state = FREE_SLOT_STATE;
  }

  ctx->links[0].prev = UINT32_MAX;
  ctx->links[ctx->slots_count - slots_per_max].next = UINT32_MAX;

  return ctx;
}

void bda_remove_from_pool(BuddyCtx *ctx, uint32_t slot_index) {
  if (ctx->links[slot_index].prev != UINT32_MAX) {
    ctx->links[ctx->links[slot_index].prev].next = ctx->links[slot_index].next;
  } else {
    ctx->pools[ctx->slots[slot_index].pool_index].head =
        ctx->links[slot_index].next;
    if (ctx->links[slot_index].next == UINT32_MAX)
      ctx->pools[ctx->slots[slot_index].pool_index].tail = UINT32_MAX;
  }

  if (ctx->links[slot_index].next != UINT32_MAX) {
    ctx->links[ctx->links[slot_index].next].prev = ctx->links[slot_index].prev;
  } else {
    ctx->pools[ctx->slots[slot_index].pool_index].tail =
        ctx->links[slot_index].prev;
    if (ctx->links[slot_index].prev == UINT32_MAX)
      ctx->pools[ctx->slots[slot_index].pool_index].head = UINT32_MAX;
  }

  ctx->slots[slot_index].pool_index = UINT8_MAX;
}

void bda_add_to_pool(BuddyCtx *ctx, uint32_t slot_index, uint8_t pool_index) {
  ctx->links[slot_index].prev = UINT32_MAX;

  if (ctx->pools[pool_index].head == UINT32_MAX) {
    ctx->pools[pool_index].tail = slot_index;
    ctx->links[slot_index].next = UINT32_MAX;
  } else {
    ctx->links[slot_index].next = ctx->pools[pool_index].head;
    ctx->links[ctx->pools[pool_index].head].prev = slot_index;
  }

  ctx->pools[pool_index].head = slot_index;
}

void *bda_alloc(IAllocator *all, size_t size) {
  if (all == NULL || size == 0)
    return NULL;

  BuddyCtx *ctx = (BuddyCtx *)all->ctx;
  size = (size + 7) & ~7;
  uint8_t size_log2 = ceil(log2(size));

  if (size_log2 > ctx->max_alloc_size_log2)
    return NULL;

  if (size_log2 < ctx->min_alloc_size_log2)
    size_log2 = ctx->min_alloc_size_log2;

  uint8_t desired_pool_index = size_log2 - ctx->min_alloc_size_log2;
  uint8_t pool_index = desired_pool_index;
  uint32_t slot_index = UINT32_MAX;

  for (; pool_index < ctx->pool_count; pool_index += 1) {
    slot_index = ctx->pools[pool_index].head;
    if (slot_index != UINT32_MAX)
      break;
  }

  if (slot_index == UINT32_MAX)
    return NULL;

  bda_remove_from_pool(ctx, slot_index);

  while (pool_index > desired_pool_index) {
    pool_index -= 1;
    uint32_t buddy_index = slot_index ^ (1U << pool_index);
    ctx->slots[buddy_index].pool_index = pool_index;
    ctx->slots[buddy_index].state = FREE_SLOT_STATE;
    bda_add_to_pool(ctx, buddy_index, pool_index);
  }

  ctx->slots[slot_index].state = BUSY_SLOT_STATE;
  ctx->slots[slot_index].pool_index = pool_index;

  return (void *)((uint64_t)ctx->start +
                  (slot_index << ctx->min_alloc_size_log2));
}

void bda_free(IAllocator *all, void *ptr) {
  if (ptr == NULL || all == NULL)
    return;

  BuddyCtx *ctx = all->ctx;
  uint64_t offset = (uint64_t)ptr - (uint64_t)ctx->start;
  uint32_t slot_index = offset >> ctx->min_alloc_size_log2;

  ctx->slots[slot_index].state = FREE_SLOT_STATE;
  uint8_t pool_index = ctx->slots[slot_index].pool_index;

  for (; pool_index < ctx->pool_count - 1; pool_index++) {
    uint32_t buddy_index = slot_index ^ (1U << pool_index);
    if (ctx->slots[buddy_index].state != FREE_SLOT_STATE ||
        ctx->slots[buddy_index].pool_index != pool_index) {
      break;
    }
    ctx->slots[buddy_index].state = NOT_USED_SLOT_STATE;
    bda_remove_from_pool(ctx, buddy_index);

    uint32_t old_slot = slot_index;
    slot_index &= UINT32_MAX << (pool_index + 1);

    if (old_slot != slot_index) {
      ctx->slots[old_slot].state = NOT_USED_SLOT_STATE;
      ctx->slots[old_slot].pool_index = UINT8_MAX;
    }
  }

  ctx->slots[slot_index].state = FREE_SLOT_STATE;
  ctx->slots[slot_index].pool_index = pool_index;

  bda_add_to_pool(ctx, slot_index, pool_index);
}

void *bda_realloc(IAllocator *all, void *ptr, size_t new_size) {
  if (all == NULL)
    return NULL;

  BuddyCtx *ctx = all->ctx;

  if (ptr == NULL) {
    return bda_alloc(all, new_size);
  } else if (new_size == 0) {
    bda_free(all, ptr);
    return NULL;
  }

  uint64_t offset = (uint64_t)ptr - (uint64_t)ctx->start;
  uint32_t slot_index = offset >> ctx->min_alloc_size_log2;
  uint32_t slot_size =
      1 << (ctx->min_alloc_size_log2 + ctx->slots[slot_index].pool_index);

  if (new_size <= slot_size) {
    return ptr;
  }

  void *new_ptr = bda_alloc(all, new_size);
  if (new_ptr == NULL) {
    return NULL;
  }

  memcpy(new_ptr, ptr, slot_size);

  bda_free(all, ptr);

  return new_ptr;
}

void bda_free_allocator(IAllocator *all) {
  BuddyCtx *ctx = all->ctx;
  free(ctx->links);
  free(ctx->pools);
  free(ctx->slots);
  free(ctx->start);
  free(ctx);
}

void bda_reset(IAllocator *all) {
  if (all == NULL)
    return;

  BuddyCtx *ctx = all->ctx;
  uint8_t total_range_log2 = ctx->total_range_log2,
          min_alloc_size_log2 = ctx->min_alloc_size_log2,
          max_alloc_size_log2 = ctx->max_alloc_size_log2;
  bda_free_allocator(all);
  *all = bda_create_allocator(total_range_log2, min_alloc_size_log2,
                              max_alloc_size_log2);
}

IAllocator bda_create_allocator(uint8_t total_range_size_log2,
                                uint8_t min_alloc_size_log2,
                                uint8_t max_alloc_size_log2) {
  BuddyCtx *ctx = bda_create_ctx(total_range_size_log2, min_alloc_size_log2,
                                 max_alloc_size_log2);
  return (struct IAllocator){bda_alloc, bda_free, bda_realloc, bda_reset, ctx};
}
