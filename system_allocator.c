#include "system_allocator.h"

#include <stdlib.h>

void *sys_alloc(IAllocator *self, size_t size) {
  (void)self;
  return malloc(size);
}

void sys_free(IAllocator *self, void *ptr) {
  (void)self;
  free(ptr);
}

void *sys_realloc(IAllocator *self, void *ptr, size_t new_size) {
  (void)self;
  return realloc(ptr, new_size);
}

void sys_reset(IAllocator *self) { (void)self; }

IAllocator create_sys_allocator(void) {
  IAllocator alloc;

  alloc.alloc = sys_alloc;
  alloc.free = sys_free;
  alloc.realloc = sys_realloc;
  alloc.reset = sys_reset;
  alloc.ctx = NULL;

  return alloc;
}
