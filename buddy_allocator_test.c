#include "buddy_allocator.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

void test_null_alloc(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  void *p = a.alloc(NULL, 100);
  assert(p == NULL);
  bda_free_allocator(&a);
}

void test_null_free(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  a.free(NULL, (void *)0x1000);
  bda_free_allocator(&a);
}

void test_null_realloc(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  void *p = a.realloc(NULL, (void *)0x1000, 100);
  assert(p == NULL);
  bda_free_allocator(&a);
}

void test_null_reset(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  a.reset(NULL);
  bda_free_allocator(&a);
}

void test_alloc_zero(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  assert(a.ctx != NULL);

  void *p = a.alloc(&a, 0);
  assert(p == NULL);
  a.free(&a, p);

  bda_free_allocator(&a);
}

void test_alloc_max(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  assert(a.ctx != NULL);

  void *p = a.alloc(&a, 256);
  assert(p != NULL);
  a.free(&a, p);

  void *p2 = a.alloc(&a, 257);
  assert(p2 == NULL);

  bda_free_allocator(&a);
}

void test_alloc_exhaust(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  assert(a.ctx != NULL);

  void *ptrs[256];
  for (int i = 0; i < 256; i++) {
    ptrs[i] = a.alloc(&a, 16);
    assert(ptrs[i] != NULL);
  }
  void *extra = a.alloc(&a, 16);
  assert(extra == NULL);

  a.free(&a, ptrs[0]);
  extra = a.alloc(&a, 16);
  assert(extra != NULL);
  a.free(&a, extra);

  for (int i = 1; i < 256; i++)
    a.free(&a, ptrs[i]);

  bda_free_allocator(&a);
}

void test_alignment(void) {
  IAllocator a = bda_create_allocator(12, 3, 8);
  assert(a.ctx != NULL);

  void *p1 = a.alloc(&a, 1);
  void *p2 = a.alloc(&a, 15);
  void *p3 = a.alloc(&a, 100);

  assert(((uintptr_t)p1 & 7) == 0);
  assert(((uintptr_t)p2 & 7) == 0);
  assert(((uintptr_t)p3 & 7) == 0);

  a.free(&a, p1);
  a.free(&a, p2);
  a.free(&a, p3);

  bda_free_allocator(&a);
}

void test_free_null_ptr(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  assert(a.ctx != NULL);

  a.free(&a, NULL);

  bda_free_allocator(&a);
}

void test_realloc_null_ptr(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  assert(a.ctx != NULL);

  void *p = a.realloc(&a, NULL, 16);
  assert(p != NULL);
  a.free(&a, p);

  bda_free_allocator(&a);
}

void test_realloc_zero_size(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  assert(a.ctx != NULL);

  void *p = a.alloc(&a, 32);
  assert(p != NULL);

  void *p2 = a.realloc(&a, p, 0);
  assert(p2 == NULL);

  void *p3 = a.alloc(&a, 32);
  assert(p3 != NULL);
  a.free(&a, p3);

  bda_free_allocator(&a);
}

void test_data_preservation(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  assert(a.ctx != NULL);

  void *p = a.alloc(&a, 16);
  assert(p != NULL);
  memset(p, 0xAB, 16);

  void *new_p = a.realloc(&a, p, 100);
  assert(new_p != NULL);

  unsigned char *buf = (unsigned char *)new_p;
  for (int i = 0; i < 16; i++)
    assert(buf[i] == 0xAB);

  a.free(&a, new_p);
  bda_free_allocator(&a);
}

void test_buddy_merge(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  assert(a.ctx != NULL);

  void *p1 = a.alloc(&a, 16);
  void *p2 = a.alloc(&a, 16);
  assert(p1 != NULL && p2 != NULL);
  assert((uint8_t *)p2 - (uint8_t *)p1 == 16);

  a.free(&a, p1);
  a.free(&a, p2);

  void *p3 = a.alloc(&a, 32);
  assert(p3 != NULL);
  assert((uintptr_t)p3 == (uintptr_t)p1);

  a.free(&a, p3);
  bda_free_allocator(&a);
}

void test_reset(void) {
  IAllocator a = bda_create_allocator(12, 4, 8);
  assert(a.ctx != NULL);

  void *big = a.alloc(&a, 256);
  assert(big != NULL);
  a.free(&a, big);

  a.reset(&a);

  void *big2 = a.alloc(&a, 256);
  assert(big2 != NULL);
  a.free(&a, big2);

  bda_free_allocator(&a);
}

int main(void) {
  test_null_alloc();
  test_null_free();
  test_null_realloc();
  test_null_reset();
  test_alloc_zero();
  test_alloc_max();
  test_alloc_exhaust();
  test_alignment();
  test_free_null_ptr();
  test_realloc_null_ptr();
  test_realloc_zero_size();
  test_data_preservation();
  test_buddy_merge();
  test_reset();

  return 0;
}
