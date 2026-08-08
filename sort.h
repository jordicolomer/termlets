#ifndef SORT_H
#define SORT_H
#include "file_manager.h"

#define SORT_BY_PATH 0
#define SORT_BY_DATE 1
#define SORT_BY_SIZE 2

FileItemWindow* mergeSort(FileItemWindow *head, int sort_by, int reversed);

#endif
