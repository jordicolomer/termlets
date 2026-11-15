#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <stdio.h>

int max(int a, int b);
int min(int a, int b);
int ends_with_ignore_case(const char *str, const char *suffix);

void *my_malloc(size_t size);
void my_free(void * ptr);
char *my_strdup(const char *s);

int encode_utf8(uint32_t c, char *buf);
char *strcasestr(const char *haystack, const char *needle);
int count_digits(int n);
int contains_ignore_case(const char *str, const char *sub);
char *strrstr(const char *str, const char *start, const char *sub);
void human_size(off_t size, char *buf, size_t buflen);

#endif
