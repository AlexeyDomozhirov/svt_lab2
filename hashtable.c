#include "hashtable.h"

Entry *new_entry(IAllocator *allocator, void *key, void *val, Entry *next) {
  Entry *n = allocator->alloc(allocator, sizeof(Entry));
  if (n == NULL) {
    return NULL;
  }
  n->key = key;
  n->value = val;
  n->next = next;
  return n;
}

HashTable *ht_new(size_t size, IAllocator *allocator,
                  size_t (*hash)(const void *key),
                  bool (*eq_key)(const void *left, const void *right),
                  void (*free_key)(void *key),
                  void (*free_value)(void *value)) {
  size_t alloc_size = sizeof(Entry *) * size;
  if (size != 0 && alloc_size / sizeof(Entry *) != size) {
    return NULL;
  }
  Entry **buckets = allocator->alloc(allocator, alloc_size);
  if (buckets == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < size; i++) {
    buckets[i] = NULL;
  }

  HashTable *ht = allocator->alloc(allocator, sizeof(HashTable));
  if (ht == NULL) {
    allocator->free(allocator, buckets);
    return NULL;
  }

  ht->size = size;
  ht->buckets = buckets;
  ht->allocator = allocator;
  ht->hash = hash;
  ht->eq_key = eq_key;
  ht->free_key = free_key;
  ht->free_value = free_value;

  return ht;
}

InsertResult ht_insert(HashTable *hashtable, void *key, void *val) {
  size_t pos = hashtable->hash(key) % hashtable->size;

  Entry *cur = hashtable->buckets[pos];
  if (cur == NULL) {
    Entry *new = new_entry(hashtable->allocator, key, val, NULL);
    if (new == NULL) {
      return INS_ALLOCATION_ERROR;
    }
    hashtable->buckets[pos] = new;
    return INS_SUCC;
  }

  while (true) {
    if (hashtable->eq_key(cur->key, key)) {
      return INS_KEY_COLLISION;
    }

    if (cur->next == NULL)
      break;

    cur = cur->next;
  }
  cur->next = new_entry(hashtable->allocator, key, val, NULL);

  if (cur->next == NULL) {
    return INS_ALLOCATION_ERROR;
  }

  return INS_SUCC;
}

void *ht_get(HashTable *hashtable, void *key) {
  size_t pos = hashtable->hash(key) % hashtable->size;
  Entry *cur = hashtable->buckets[pos];

  while (cur != NULL) {
    if (hashtable->eq_key(cur->key, key)) {
      return cur->value;
    }
    cur = cur->next;
  }
  return NULL;
}

RemoveResult ht_remove(HashTable *hashtable, const void *key) {
  size_t pos = hashtable->hash(key) % hashtable->size;
  Entry *cur = hashtable->buckets[pos];

  Entry *prev = NULL;
  while (cur != NULL && !hashtable->eq_key(cur->key, key)) {
    prev = cur;
    cur = cur->next;
  }

  if (cur == NULL)
    return REM_NO_SUCH_KEY;

  hashtable->free_key(cur->key);
  hashtable->free_value(cur->value);

  if (prev == NULL)
    hashtable->buckets[pos] = cur->next;
  else
    prev->next = cur->next;

  hashtable->allocator->free(hashtable->allocator, cur);

  return REM_SUCC;
}

void ht_free(HashTable *hashtable) {
  for (size_t i = 0; i < hashtable->size; i++) {
    Entry *cur = hashtable->buckets[i];
    while (cur != NULL) {
      Entry *next = cur->next;

      hashtable->free_key(cur->key);
      hashtable->free_value(cur->value);

      hashtable->allocator->free(hashtable->allocator, cur);

      cur = next;
    }
  }
  hashtable->allocator->free(hashtable->allocator, hashtable->buckets);
  hashtable->allocator->free(hashtable->allocator, hashtable);
}
