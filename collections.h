#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include <stdint.h>

typedef struct ArrayListNode ArrayListNode;

typedef struct ArrayListNode {
    ArrayListNode * next;
    char data[];
} ArrayListNode;

typedef struct ArrayListIterator {
  ArrayListNode * ar;
  int idx;
} ArrayListIterator;

typedef struct ArrayList {
  ArrayListIterator first;
  ArrayListIterator last;
  uint32_t elementSize;
  uint32_t arraySize;
  uint32_t count;
} ArrayList;

void ArrayList_init(ArrayList * self, uint32_t elementSize, uint32_t arraySize);
int ArrayListIteratorValid(ArrayList * self, ArrayListIterator * ite);
void ArrayListIteratorNext(ArrayList * self, ArrayListIterator * ite);
char * ArrayListIteratorElement(ArrayList * self, ArrayListIterator * ite);
void ArrayList_append(ArrayList * self, char * data);
void ArrayList_reset(ArrayList * self);

#endif
