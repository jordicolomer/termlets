#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#include "logger.h"

int max(int a, int b) { return (a > b) ? a : b; }

int min(int a, int b) { return (a < b) ? a : b; }

int ends_with_ignore_case(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
        return 0;

    return strcasecmp(str + str_len - suffix_len, suffix) == 0;
}

#include "common.h"
#include <malloc/malloc.h>

void *my_malloc(size_t size) {
    void *ptr = malloc(size);
    LOG_INFO("my_malloc %d %d %p", size, malloc_size(ptr), ptr);
    memset(ptr, 0, size); // Zero-initialize to prevent garbage values
    allocated_memory += malloc_size(ptr);
    return ptr;
}

void my_free(void * ptr) {
    size_t size = malloc_size(ptr);
    LOG_INFO("my_free %d %p", size, ptr);
    allocated_memory -= size;
    free(ptr);
}

#include <string.h>   // strlen, memcpy
#include <stdlib.h>   // for size_t, NULL

char *my_strdup(const char *s) {
    if (s == NULL) {
        return NULL;
    }

    size_t len = strlen(s) + 1;          // +1 for the terminating '\0'
    char *dup = my_malloc(len);          // uses your tracked allocator

    if (dup != NULL) {
        memcpy(dup, s, len);             // or strcpy(dup, s);
    }

    return dup;
}

/* Helper function to encode a unicode codepoint to UTF-8 */
int encode_utf8(uint32_t c, char *buf) {
    if (c == 0) {
        buf[0] = ' ';
        return 1;
    } else if (c == (uint32_t)-1) {
        /* skip continuation cell for wide characters */
        return 0;
    } else if (c < 0x80) {
        buf[0] = (char)c;
        return 1;
    } else if (c < 0x800) {
        buf[0] = 0xC0 | (c >> 6);
        buf[1] = 0x80 | (c & 0x3F);
        return 2;
    } else if (c < 0x10000) {
        buf[0] = 0xE0 | (c >> 12);
        buf[1] = 0x80 | ((c >> 6) & 0x3F);
        buf[2] = 0x80 | (c & 0x3F);
        return 3;
    } else {
        buf[0] = 0xF0 | (c >> 18);
        buf[1] = 0x80 | ((c >> 12) & 0x3F);
        buf[2] = 0x80 | ((c >> 6) & 0x3F);
        buf[3] = 0x80 | (c & 0x3F);
        return 4;
    }
}

char *strcasestr(const char *haystack, const char *needle) {
    if (*needle == '\0')
        return (char *)haystack;

    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;

        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++;
            n++;
        }

        if (*n == '\0')
            return (char *)haystack;
    }

    return NULL;
}

int count_digits(int n) {
    int count = 0;

    do {
        count++;
        n /= 10;
    } while (n != 0);

    return count;
}

int contains_ignore_case(const char *str, const char *sub) {
    if (*sub == '\0')
        return 1;

    for (; *str; str++) {
        const char *s = str;
        const char *p = sub;

        while (*s && *p && tolower((unsigned char)*s) == tolower((unsigned char)*p)) {
            s++;
            p++;
        }

        if (*p == '\0')
            return 1;
    }

    return 0;
}

char *strrstr(const char *str, const char *start, const char *sub) {
    size_t len = strlen(sub);

    if (len == 0)
        return (char *)start;

    const char *p = start - len;

    while (p >= str) {
        size_t i = 0;

        while (i < len && p[i] == sub[i])
            i++;

        if (i == len)
            return (char *)p;

        p--;
    }

    return NULL;
}

void human_size(off_t size, char *buf, size_t buflen) {
    const char *units[] = {"Bytes", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double s = (double)size;

    while (s >= 1024 && unit < 4) {
        s /= 1024;
        unit++;
    }

    if (unit == 0)
        snprintf(buf, buflen, "%.0f %s", s, units[unit]);
    else
        snprintf(buf, buflen, "%.1f %s", s, units[unit]);
}
