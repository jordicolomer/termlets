#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

int copy_file(const char *src, const char *dst);
int file_exists(const char *path);
int remove_recursive(const char *path);
int copy_recursive(const char *src, const char *dst);

#endif