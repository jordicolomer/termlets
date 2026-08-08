#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "sort.h"


FileItemWindow* split(FileItemWindow *head)
{
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

FileItemWindow* merge(FileItemWindow *first, FileItemWindow *second, int sort_by, int reversed)
{
    if (!first)
        return second;

    if (!second)
        return first;

    int smaller = 0;
    if (sort_by == SORT_BY_PATH) smaller = strcmp(first->path, second->path) <= 0;
    if (sort_by == SORT_BY_DATE) smaller = first->date <= second->date;
    if (sort_by == SORT_BY_SIZE) smaller = first->size <= second->size;
    if (reversed) smaller = 1-smaller;

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

FileItemWindow* mergeSort(FileItemWindow *head, int sort_by, int reversed)
{
    if (!head || !head->win.next)
        return head;

    FileItemWindow *second = split(head);

    head = mergeSort(head, sort_by, reversed);
    second = mergeSort(second, sort_by, reversed);

    return merge(head, second, sort_by, reversed);
}

/*int main(){
  Node * node1 = malloc(sizeof * node1); node1->data = 3;
  Node * node2 = malloc(sizeof * node2); node2->data = 1;
  Node * node3 = malloc(sizeof * node3); node3->data = 4;
  Node * node4 = malloc(sizeof * node4); node4->data = 2;
  
  node1->next = node2;
  node2->next = node3;
  node3->next = node4;
  
  node2->prev = node1;
  node3->prev = node2;
  node4->prev = node3;

  Node * sorted = mergeSort(node1);
  while (sorted != NULL){
	printf("%d\n", sorted->data);
	sorted = sorted->next;
  }
  
  //printf("offsetof %d\n", offsetof(Node, prev));
  printf("offsetof %zu\n", offsetof(Node, prev));
}*/
