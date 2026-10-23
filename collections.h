#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include <stdint.h>

typedef struct ArrayListNode ArrayListNode;

typedef struct ArrayListNode {
    ArrayListNode *next;
    ArrayListNode *prev;
    char data[];
} ArrayListNode;

typedef struct ArrayListIterator {
    ArrayListNode *ar;
    int idx;
} ArrayListIterator;

typedef struct ArrayList {
    ArrayListIterator first;
    ArrayListIterator last;
    uint32_t elementSize;
    uint32_t arraySize;
    uint32_t count;
} ArrayList;

void ArrayList_init(ArrayList *self, uint32_t elementSize, uint32_t arraySize);
int ArrayListIteratorValid(ArrayList *self, ArrayListIterator *ite);
void ArrayListIteratorNext(ArrayList *self, ArrayListIterator *ite);
void ArrayListIteratorPrev(ArrayList *self, ArrayListIterator *ite);
int ArrayListIteratorIsFirst(ArrayList *self, ArrayListIterator *ite);
char *ArrayListIteratorElement(ArrayList *self, ArrayListIterator *ite);
void ArrayList_append(ArrayList *self, char *data);
void ArrayList_reset(ArrayList *self);

#include "uthash/uthash.h"

typedef struct {
    char *key;
    size_t key_len;

    UT_hash_handle hh;
} Entry;

Entry *hashmap_put(char *key, int key_size);
Entry *hashmap_get(char *key, int key_size);

#endif
