#include "allocator.h"
#include "buddy_allocator.h"
#include "hashtable.h"
#include "system_allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool int_eq(const void *a, const void *b) {
  return (int)(intptr_t)a == (int)(intptr_t)b;
}

size_t int_hash(const void *key) { return (size_t)(intptr_t)key; }

void dummy_free(void *p) { (void)p; }

void test_create_destroy(IAllocator *alloc) {
  HashTable *ht = ht_new(16, alloc, int_hash, int_eq, dummy_free, dummy_free);
  assert(ht != NULL);
  ht_free(ht);
}

void test_insert_get_basic(IAllocator *alloc) {
  HashTable *ht = ht_new(8, alloc, int_hash, int_eq, dummy_free, dummy_free);
  assert(ht != NULL);

  InsertResult ins = ht_insert(ht, (void *)(intptr_t)1, (void *)(intptr_t)42);
  assert(ins == INS_SUCC);

  void *got = ht_get(ht, (void *)(intptr_t)1);
  assert(got == (void *)(intptr_t)42);

  ht_free(ht);
}

void test_insert_duplicate(IAllocator *alloc) {
  HashTable *ht = ht_new(8, alloc, int_hash, int_eq, dummy_free, dummy_free);
  assert(ht != NULL);

  InsertResult r1 = ht_insert(ht, (void *)(intptr_t)10, (void *)1);
  assert(r1 == INS_SUCC);

  InsertResult r2 = ht_insert(ht, (void *)(intptr_t)10, (void *)2);
  assert(r2 == INS_KEY_COLLISION);

  ht_free(ht);
}

void test_get_missing(IAllocator *alloc) {
  HashTable *ht = ht_new(8, alloc, int_hash, int_eq, dummy_free, dummy_free);
  assert(ht != NULL);

  void *val = ht_get(ht, (void *)(intptr_t)404);
  assert(val == NULL);

  ht_free(ht);
}

void test_remove_existing(IAllocator *alloc) {
  HashTable *ht = ht_new(8, alloc, int_hash, int_eq, dummy_free, dummy_free);
  assert(ht != NULL);

  ht_insert(ht, (void *)(intptr_t)7, (void *)(intptr_t)77);
  RemoveResult rem = ht_remove(ht, (void *)(intptr_t)7);
  assert(rem == REM_SUCC);

  void *got = ht_get(ht, (void *)(intptr_t)7);
  assert(got == NULL);

  ht_free(ht);
}

void test_remove_missing(IAllocator *alloc) {
  HashTable *ht = ht_new(8, alloc, int_hash, int_eq, dummy_free, dummy_free);
  assert(ht != NULL);

  RemoveResult rem = ht_remove(ht, (void *)(intptr_t)999);
  assert(rem == REM_NO_SUCH_KEY);

  ht_free(ht);
}

void test_many_insert_and_lookup(IAllocator *alloc) {
  const int N = 1000;
  HashTable *ht = ht_new(256, alloc, int_hash, int_eq, dummy_free, dummy_free);
  assert(ht != NULL);

  for (int i = 0; i < N; i++) {
    InsertResult r = ht_insert(ht, (void *)(intptr_t)i, (void *)(intptr_t)i);
    assert(r == INS_SUCC);
  }

  for (int i = 0; i < N; i++) {
    void *v = ht_get(ht, (void *)(intptr_t)i);
    assert(v == (void *)(intptr_t)i);
  }

  assert(ht_get(ht, (void *)(intptr_t)N) == NULL);

  ht_free(ht);
}

void test_many_insert_and_remove(IAllocator *alloc) {
  const int N = 500;
  HashTable *ht = ht_new(128, alloc, int_hash, int_eq, dummy_free, dummy_free);
  assert(ht != NULL);

  for (int i = 0; i < N; i++) {
    ht_insert(ht, (void *)(intptr_t)i, (void *)(intptr_t)i);
  }

  for (int i = 0; i < N; i += 2) {
    RemoveResult r = ht_remove(ht, (void *)(intptr_t)i);
    assert(r == REM_SUCC);
  }

  for (int i = 0; i < N; i++) {
    void *v = ht_get(ht, (void *)(intptr_t)i);
    if (i % 2 == 0) {
      assert(v == NULL);
    } else {
      assert(v == (void *)(intptr_t)i);
    }
  }

  ht_free(ht);
}

void test_hashtable_with_allocator(IAllocator *alloc) {
  test_create_destroy(alloc);
  test_insert_get_basic(alloc);
  test_insert_duplicate(alloc);
  test_get_missing(alloc);
  test_remove_existing(alloc);
  test_remove_missing(alloc);
  test_many_insert_and_lookup(alloc);
  test_many_insert_and_remove(alloc);
}

int main(void) {
  {
    IAllocator all = bda_create_allocator(16, 3, 11);
    test_hashtable_with_allocator(&all);
    bda_free_allocator(&all);
  }
  {
    IAllocator all = create_sys_allocator();
    test_hashtable_with_allocator(&all);
  }
  return 0;
}
