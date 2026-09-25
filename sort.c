#ifdef _WIN32
/* Disable strict pointer type warnings on Windows for this file */
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "sort.h"

FileItemWindow *split(FileItemWindow *head) {
    FileItemWindow *fast = head;
    FileItemWindow *slow = head;

    while (fast->win.next && fast->win.next->next) {
        fast = fast->win.next->next;
        slow = slow->win.next;
    }

    FileItemWindow *second = slow->win.next;
    slow->win.next = NULL;

    if (second)
        second->win.prev = NULL;

    return second;
}

FileItemWindow *merge(FileItemWindow *first, FileItemWindow *second, int sort_by, int reversed) {
    if (!first)
        return second;

    if (!second)
        return first;

    int smaller = 0;
    if (sort_by == SORT_BY_PATH)
        smaller = strcmp(first->path, second->path) <= 0;
    if (sort_by == SORT_BY_DATE)
        smaller = first->date <= second->date;
    if (sort_by == SORT_BY_SIZE)
        smaller = first->size <= second->size;
    if (reversed)
        smaller = 1 - smaller;

    if (smaller) {

        first->win.next = merge(first->win.next, second, sort_by, reversed);

        if (first->win.next)
            first->win.next->prev = first;

        first->win.prev = NULL;
        return first;
    }

    second->win.next = merge(first, second->win.next, sort_by, reversed);

    if (second->win.next)
        second->win.next->prev = second;

    second->win.prev = NULL;
    return second;
}

FileItemWindow *mergeSort(FileItemWindow *head, int sort_by, int reversed) {
    if (!head || !head->win.next)
        return head;

    FileItemWindow *second = split(head);

    head = mergeSort(head, sort_by, reversed);
    second = mergeSort(second, sort_by, reversed);

    return merge(head, second, sort_by, reversed);
}
