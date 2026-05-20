#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "allocator.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Entry {
  void *key;
  void *value;
  struct Entry *next;
} Entry;

typedef struct HashTable {
  size_t size;
  Entry **buckets;
  IAllocator *allocator;
  size_t (*hash)(const void *key);
  bool (*eq_key)(const void *left, const void *right);
  void (*free_key)(void *key);
  void (*free_value)(void *value);
} HashTable;

HashTable *ht_new(size_t size, IAllocator *allocator,
                  size_t (*hash)(const void *key),
                  bool (*eq_key)(const void *left, const void *right),
                  void (*free_key)(void *key), void (*free_value)(void *value));

typedef enum { INS_SUCC, INS_ALLOCATION_ERROR, INS_KEY_COLLISION } InsertResult;

InsertResult ht_insert(HashTable *hashtable, void *key, void *val);

void *ht_get(HashTable *hashtable, void *key);

typedef enum { REM_SUCC, REM_NO_SUCH_KEY } RemoveResult;

RemoveResult ht_remove(HashTable *hashtable, const void *key);

void ht_free(HashTable *hashtable);

#endif
