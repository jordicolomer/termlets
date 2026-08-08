#include <string.h>

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
