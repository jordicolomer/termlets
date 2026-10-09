#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "collections.h"
#include "logger.h"

void ArrayList_init(ArrayList *self, uint32_t elementSize, uint32_t arraySize) {
    self->elementSize = elementSize;
    self->arraySize = arraySize;
    self->first.ar = NULL;
    self->first.idx = 0;
    self->last.ar = NULL;
    self->last.idx = 0;
    self->count = 0;
}

void _ArrayList_add_node(ArrayList *self) {
    ArrayListNode *node = malloc(sizeof(ArrayListNode) + self->elementSize * self->arraySize);
    node->next = NULL;
    if (self->first.ar == NULL) {
        self->first.ar = node;
    }
    if (self->last.ar != NULL) {
        self->last.ar->next = node;
    }
    self->last.ar = node;
}

int _ArrayList_is_full(ArrayList *self) {
    if (self->last.ar == NULL)
        return 1;
    if (self->last.idx >= self->arraySize - 1)
        return 1;
    return 0;
}

void ArrayListIteratorNext(ArrayList *self, ArrayListIterator *ite) {
    ite->idx++;
    if (ite->idx >= self->arraySize) {
        ite->ar = ite->ar->next;
        ite->idx = 0;
    }
}

int ArrayListIteratorValid(ArrayList *self, ArrayListIterator *ite) {
    if (ite->ar == NULL)
        return 0;
    if (ite->ar == self->last.ar && ite->idx > self->last.idx)
        return 0;
    return 1;
}

char *ArrayListIteratorElement(ArrayList *self, ArrayListIterator *ite) {
    return ite->ar->data + ite->idx * self->elementSize;
}

void ArrayList_append(ArrayList *self, char *data) {
    if (_ArrayList_is_full(self) == 1) {
        if (self->last.ar && self->last.ar->next != NULL)
            self->last.ar = self->last.ar->next;
        else
            _ArrayList_add_node(self);
        self->last.idx = 0;
    } else {
        if (self->count != 0)
            self->last.idx++;
    }
    char *elem = ArrayListIteratorElement(self, &self->last);
    // LOG_INFO("ArrayList_append: %p", elem);
    memcpy(elem, data, self->elementSize);
    self->count++;
}

void ArrayList_reset(ArrayList *self) {
    self->last.ar = self->first.ar;
    self->last.idx = 0;
    self->count = 0;
}

// hashmap

#include "uthash/uthash.h"

typedef struct {
    char *key;
    size_t key_len;
    int value;

    UT_hash_handle hh;
} Entry;

Entry *map;

void hashmap_put(char *key, int key_size, int value) {
    Entry *e = malloc(sizeof(*e));
    e->key = malloc(key_size);
    memcpy(e->key, key, key_size);
    e->key_len = key_size;
    e->value = value;

    HASH_ADD_KEYPTR(hh, map, e->key, e->key_len, e);
}

int hashmap_get(char *key, int key_size) {
    Entry *found;

    HASH_FIND(hh, map, key, key_size, found);

    if (found) {
        // printf("value = %d\n", found->value);
        return found->value;
    }

    return -1;

    // HASH_DEL(map, e);
    // free(e->key);
    // free(e);
}
