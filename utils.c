#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "logger.h"

int max(int a, int b)
{
  return (a > b) ? a : b;
}

int min(int a, int b)
{
  return (a < b) ? a : b;
}

int ends_with_ignore_case(const char *str, const char *suffix)
{
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
        return 0;

    return strcasecmp(str + str_len - suffix_len, suffix) == 0;
}

void * my_malloc(int size){
  LOG_INFO("my_malloc %d", size);
  void *ptr = malloc(size);
  memset(ptr, 0, size);  // Zero-initialize to prevent garbage values
  return ptr;
}


/* Helper function to encode a unicode codepoint to UTF-8 */
int encode_utf8(uint32_t c, char *buf)
{
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

