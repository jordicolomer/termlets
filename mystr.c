#include <stdio.h>
#include <string.h>
#include <utf8proc.h>
#include "uwidth.h"
#include "mystr.h"
#include "logger.h"
#include "common.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "wcwidth/grapheme.h"

// Detect terminal type from environment variables
// Returns a cached terminal string suitable for wcwidth functions
const char *get_terminal_type(void) {
  //return "kitty2";
    static const char *cached_terminal = NULL;

    if (cached_terminal != NULL)
        return cached_terminal;

    const char *term_program = getenv("TERM_PROGRAM");
    const char *term = getenv("TERM");

    // Check TERM_PROGRAM first (macOS and some other systems)
    if (term_program != NULL) {
        if (strstr(term_program, "iTerm") != NULL) {
            cached_terminal = "iterm2";
            LOG_INFO("Terminal detected: %s (from TERM_PROGRAM=%s)", cached_terminal, term_program);
            return cached_terminal;
        }
        if (strstr(term_program, "Apple_Terminal") != NULL) {
            cached_terminal = "apple";
            LOG_INFO("Terminal detected: %s (from TERM_PROGRAM=%s)", cached_terminal, term_program);
            return cached_terminal;
        }
        if (strstr(term_program, "vscode") != NULL) {
            cached_terminal = "konsole";  // VSCode behaves similar to konsole
            LOG_INFO("Terminal detected: %s (from TERM_PROGRAM=%s)", cached_terminal, term_program);
            return cached_terminal;
        }
        if (strstr(term_program, "ghostty") != NULL) {
            cached_terminal = "ghostty";
            LOG_INFO("Terminal detected: %s (from TERM_PROGRAM=%s)", cached_terminal, term_program);
            return cached_terminal;
        }
    }

    // Check TERM environment variable
    if (term != NULL) {
        if (strstr(term, "kitty") != NULL) {
            cached_terminal = "kitty";
            LOG_INFO("Terminal detected: %s (from TERM=%s)", cached_terminal, term);
            return cached_terminal;
        }
        if (strstr(term, "alacritty") != NULL) {
            cached_terminal = "konsole";  // Alacritty is similar to konsole
            LOG_INFO("Terminal detected: %s (from TERM=%s)", cached_terminal, term);
            return cached_terminal;
        }
        if (strstr(term, "konsole") != NULL) {
            cached_terminal = "konsole";
            LOG_INFO("Terminal detected: %s (from TERM=%s)", cached_terminal, term);
            return cached_terminal;
        }
        if (strstr(term, "xterm") != NULL) {
            cached_terminal = "alacritty";
            LOG_INFO("Terminal detected: %s (from TERM=%s)", cached_terminal, term);
            return cached_terminal;
        }
    }

    // Default fallback
    cached_terminal = "iterm2";
    LOG_INFO("Terminal detected: %s (default fallback, TERM_PROGRAM=%s, TERM=%s)",
             cached_terminal,
             term_program ? term_program : "null",
             term ? term : "null");
    return cached_terminal;
}

ssize_t utf8_iterate(const uint8_t *str, ssize_t strlen, int32_t *codepoint_ref) {
    if (str[0] < 0x80) {
        *codepoint_ref = str[0];
        return 1;
    } else if ((str[0] & 0xE0) == 0xC0) {
        *codepoint_ref = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
        return 2;
    } else if ((str[0] & 0xF0) == 0xE0) {
        *codepoint_ref = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
        return 3;
    } else if ((str[0] & 0xF8) == 0xF0) {
        *codepoint_ref = ((str[0] & 0x07) << 18) | ((str[1] & 0x3F) << 12) |
                         ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
        return 4;
    } else {
        return 1;
    }
}

ssize_t utf8_iterate_backward(const uint8_t *str) {
    str--;
    ssize_t size = 1;

    while ((str[0] & 0xC0) == 0x80) {
        str--;
        size += 1;
    }

    return size;
}

void cluster_codepoints_to_string(MyStr *str_ite, char *output, size_t output_size) {
    size_t offset = 0;
    size_t bytes_processed = 0;

    while (bytes_processed < str_ite->cluster_size && offset < output_size - 10) {
        int32_t codepoint = 0;
        ssize_t bytes_read = utf8_iterate(
            (const uint8_t *)str_ite->utf8 + str_ite->cluster_start + bytes_processed,
            str_ite->cluster_size - bytes_processed,
            &codepoint
        );

        if (bytes_read <= 0) break;

        if (bytes_processed > 0) {
            offset += snprintf(output + offset, output_size - offset, " ");
        }
        offset += snprintf(output + offset, output_size - offset, "U+%04X", codepoint);

        bytes_processed += bytes_read;
    }
}


uint32_t utf8_next_codepoint(const uint8_t *s, int *idx) {
    const uint8_t *p = s + *idx;
    uint32_t cp = 0;

    if (p[0] < 0x80) {
        cp = p[0];
        *idx += 1;
    } else if ((p[0] & 0xE0) == 0xC0) {
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        *idx += 2;
    } else if ((p[0] & 0xF0) == 0xE0) {
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        *idx += 3;
    } else if ((p[0] & 0xF8) == 0xF0) {
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        *idx += 4;
    } else {
        *idx += 1;
    }

    return cp;
}

uint32_t utf8_current_codepoint(const uint8_t *s, int idx) { return utf8_next_codepoint(s, &idx); }

uint32_t utf8_prev_codepoint(const uint8_t *start, int *idx) {
    const uint8_t *s = start + *idx;
    const uint8_t *p = s;
    uint32_t cp = 0;

    if (p <= start)
        return 0;

    // Move to the first byte of the previous UTF-8 character.
    p--;
    (*idx)--;

    while (p > start && (p[0] & 0xC0) == 0x80) {
        p--;
        (*idx)--;
    }

    if (p[0] < 0x80) {
        cp = p[0];
    } else if ((p[0] & 0xE0) == 0xC0) {
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    } else if ((p[0] & 0xF0) == 0xE0) {
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    } else if ((p[0] & 0xF8) == 0xF0) {
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
    } else {
        // Invalid UTF-8 leading byte.
        cp = 0;
    }

    //*s = p;

    return cp;
}

int orig(void) {
    printf(" # orig\n");
    const char *text = "á́ b 👨‍👩‍👧‍👦 👨‍👩‍👧‍👦 🇵🇱 abcd";

    size_t len = strlen(text);
    size_t pos = 0;

    size_t cluster_start = 0;
    utf8proc_int32_t previous = -1;
    utf8proc_int32_t state = 0;
    size_t codepoints_width = -1;

    while (pos < len) {
        utf8proc_int32_t current;

        utf8proc_ssize_t bytes =
            utf8proc_iterate((const utf8proc_uint8_t *)text + pos, len - pos, &current);
        codepoints_width += 1;
        // printf("U+%04X\n", (unsigned int)current);

        if (bytes < 0) {
            printf("Invalid UTF-8\n");
            break;
        }

        /*
         * Is there a grapheme break before `current`?
         */
        if (previous != -1 && utf8proc_grapheme_break_stateful(previous, current, &state)) {

            printf("cluster_start: %d cluster_end: %d ", cluster_start, pos);
            printf("cluster: \"%.*s\" ", (int)(pos - cluster_start), text + cluster_start);
            printf("codepoints_width: %d\n", codepoints_width);

            cluster_start = pos;
            codepoints_width = 0;
        }

        previous = current;
        pos += bytes;
    }

    /* Last cluster */
    if (cluster_start < len) {
        printf("cluster_start: %d cluster_end: %d ", cluster_start, len);
        printf("cluster: \"%.*s\"\n", (int)(len - cluster_start), text + cluster_start);
    }

    return 0;
}

uint_least32_t uw_cluster(const Uwidth_Code_Point *code_points, unsigned int length,
                          Uwidth_Profile profile) {
    Uwidth_State state;
    Uwidth_Event event;
    unsigned int index;

    uwidth_init(&state, profile);
    for (index = 0U; index < length; ++index) {
        printf("uwidth_push %d %d\n", index, code_points[index]);
        uwidth_push(&state, code_points[index], &event);
    }
    uwidth_finish(&state, &event);
    return event.width;
}

int get_width(char *s, int size);

/*int wc_len(char *text, int length) {
  return get_width(text, length);
  }*/

int wc_len(char *text, int length) {
    // int ret = uw_cluster((Uwidth_Code_Point *)text, len, uwidth_profile_east_asian);
    // printf("wc_len \"%.*s\" ret=%d len=%d \n", len,text, ret, len);
    // return ret;
    Uwidth_State state;
    Uwidth_Event event;
    unsigned int index;

    uwidth_init(&state, uwidth_profile_east_asian);
    // uwidth_init(&state, uwidth_profile_narrow);
    size_t len = strlen(text);
    size_t pos = 0;
    for (index = 0U; index < length; ++index) {
        // while (pos < len) {
        utf8proc_int32_t current;
        utf8proc_ssize_t bytes =
            utf8proc_iterate((const utf8proc_uint8_t *)text + pos, len - pos, &current);

        if (length == 2 && index == 0) {
            if (current == 0x0600)
                return 2;
            if (current == 0x0601)
                return 2;
            if (current == 0x0602)
                return 2;
            if (current == 0x0603)
                return 2;
        }
        /*if (length == 2 && index == 1) {
if (prev == 0x20 && current == 0xBBE)
return 2;
if (prev == 0x20 && current == 0xBBF)
return 2;
if (prev == 0x20 && current == 0xBC0)
return 2;
if (prev == 0x20 && current == 0xBC1)
return 2;
if (prev == 0x20 && current == 0xBC2)
return 2;
if (prev == 0x20 && current == 0xBC6)
return 2;
if (prev == 0x20 && current == 0xBC7)
return 2;
if (prev == 0x20 && current == 0xBC8)
return 2;
if (prev == 0x20 && current == 0xBCA)
return 2;
if (prev == 0x20 && current == 0xBCB)
return 2;
                }*/

        if (length == 1) {
            if (current == 9633)
                return 1;
            if (current == 9475)
                return 1;
            if (current == 0x2192)
                return 1;
            if (current == 0x25B2)
                return 1;
            if (current == 0x25BC)
                return 1;
            if (current == 0x1F3DE)
                return 1;
            if (current == 0x2193)
                return 1;
            if (current == '\t')
                return tab_width;
            if (current >= 0x0300 && current <= 0x036F)
                return 0;
            if (current <= 31 && current != '\t')
                return 0;
            if (current == 0x0085)
                return 0;
            if (current == 0x00A9)
                return 1;
            if (current == 0x00AE)
                return 1;
            // if (current == 1522) return 3;
            // if (current == 0x00A9) return 2;
        }

        uwidth_push(&state, current, &event);
        pos += bytes;
    }
    uwidth_finish(&state, &event);
    // LOG_INFO("wc_len %s %d %d", text, length, event.width);
    return event.width;
}

void test_wc_len() {
    char *text = " a";
    printf("space len %d\n", wc_len(text, 1));
}

void MyStr_init(MyStr *mystr, char *utf8) {
    mystr->utf8 = utf8;
    mystr->len_bytes = strlen(utf8);

    mystr->width_bytes = 0;
    mystr->pos_bytes = 0;
    mystr->current = -1;

    mystr->width_column = 0;
    mystr->pos_column = 0;
    mystr->pos_cluster = -1;
    // mystr->cluster_end = 0;
    mystr->cluster_start = 0;
    mystr->cluster_size = 0;
    // mystr->previous = -1;
    // mystr->state = 0;
}

int MyStr_last_codepoint(MyStr *mystr) { return !(mystr->pos_bytes < mystr->len_bytes); }

int MyStr_first_codepoint(MyStr *mystr) { return mystr->pos_bytes <= 0; }

int _MyStr_next_codepoint(MyStr *mystr) {
    if (MyStr_last_codepoint(mystr))
        return 0;
    // mystr->previous = mystr->current;
    utf8proc_ssize_t bytes =
        utf8proc_iterate((const utf8proc_uint8_t *)mystr->utf8 + mystr->pos_bytes,
                         mystr->len_bytes - mystr->pos_bytes, &mystr->current);

    if (bytes < 0) {
        printf("Invalid UTF-8\n");
    }
    mystr->pos_bytes += bytes;
    // int w = utf8proc_charwidth(mystr->current);
    // int w = wc_len(char * text, int len);
    // mystr->pos_column += w;
    // return (mystr->pos_bytes < mystr->len_bytes);
    return 1;
}

int MyStr_next_codepoint(MyStr *mystr) {
    if (MyStr_last_codepoint(mystr))
        return 0;
    // mystr->previous = mystr->current;
    mystr->pos_bytes += mystr->width_bytes;
    mystr->width_bytes = utf8_iterate((const utf8proc_uint8_t *)mystr->utf8 + mystr->pos_bytes,
                                      mystr->len_bytes - mystr->pos_bytes, &mystr->current);
    // mystr->current = utf8_next_codepoint(mystr->utf8, &mystr->pos_bytes);
    return 1;
}
// uint32_t utf8_prev_codepoint(const uint8_t *start, int *idx) {

// ssize_t utf8_iterate_backward(const uint8_t * str ){
int MyStr_prev_codepoint(MyStr *mystr) {
    // printf("MyStr_prev_codepoint0: pos_bytes:%d\n", mystr->pos_bytes);
    if (MyStr_first_codepoint(mystr))
        return 0;
    mystr->width_bytes =
        utf8_iterate_backward((const utf8proc_uint8_t *)mystr->utf8 + mystr->pos_bytes);
    // printf("MyStr_prev_codepoint1: width_bytes:%d pos_bytes:%d\n", mystr->width_bytes,
    // mystr->pos_bytes);
    mystr->pos_bytes -= mystr->width_bytes;

    // read the codepoint
    utf8_iterate((const utf8proc_uint8_t *)mystr->utf8 + mystr->pos_bytes,
                 mystr->len_bytes - mystr->pos_bytes, &mystr->current);

    // printf("MyStr_prev_codepoint2 pos_bytes:%d\n", mystr->pos_bytes);
    return 1;
}

int MyStr_is_end(MyStr *mystr) {
    // printf("MyStr_is_end mystr->cluster_start:%d, mystr->cluster_size:%d, mystr->len_bytes:%d\n",
    // mystr->cluster_start, mystr->cluster_size, mystr->len_bytes); return mystr->cluster_start +
    // mystr->cluster_size >= mystr->len_bytes;
    return mystr->cluster_start >= mystr->len_bytes;
}

#include "wcwidth/wcwidth.h"
int MyStr_next_cluster(MyStr *mystr) {
    const char *terminal = get_terminal_type();
    // input is pos_bytes only
    utf8proc_int32_t state = 0;
    // printf("MyStr_next_cluster\n");
    if (MyStr_is_end(mystr))
        return 0;
    // printf("mystr->cluster_start:%d\n", mystr->cluster_start);
    mystr->cluster_start = mystr->pos_bytes;
    mystr->pos_column += mystr->width_column;
    mystr->width_codepoints = 0;
    if (mystr->pos_cluster == -1)
        mystr->width_codepoints = -1;
    mystr->cluster_size = mystr->width_bytes;
    int previous = mystr->current;
    mystr->first_codepoint = -1;
    while (MyStr_next_codepoint(mystr)) {
        mystr->width_codepoints += 1;
        if (previous != -1) {
            if (mystr->first_codepoint == -1) {
                mystr->first_codepoint = previous;
            }
            if (utf8proc_grapheme_break_stateful(previous, mystr->current, &state)) {
                goto ret;
            }
        }
        mystr->cluster_size += mystr->width_bytes;
        previous = mystr->current;
    }
    mystr->width_codepoints += 1;
ret:
    mystr->pos_cluster += 1;
    // mystr->width_column = wc_len(mystr->utf8 + mystr->cluster_start, mystr->width_codepoints);
    mystr->width_column =
	  wcstwidth_u8(mystr->utf8 + mystr->cluster_start, mystr->cluster_size, 1, terminal); // alacritty
    if (mystr->utf8[mystr->cluster_start] == '\t' && mystr->cluster_size == 1){
        mystr->width_column = tab_width;
	}/*else if (mystr->cluster_size == 4 &&
			  memcmp(mystr->utf8 + mystr->cluster_start,
					 "\xF0\x9F\x8F\x9E", 4) == 0) {
	  mystr->width_column = 2;
	  }*/
    //printf("cluster: \"%.*s\"\n", (int)(mystr->cluster_size), mystr->utf8 + mystr->cluster_start);
	//LOG_INFO("wcstwidth_u8: %.*s %d", mystr->cluster_size, mystr->utf8 + mystr->cluster_start, mystr->width_column);
    return 1;
}

int _MyStr_prev_cluster(MyStr *mystr) {
    // rewind codepoint iterator to beginning of cluster
    // mystr->pos_bytes = mystr->cluster_start;
    /*size_t start = mystr->cluster_start;
    while (start > 0){
          mystr->pos_bytes = start;
          while (MyStr_next_codepoint(mystr)) {
            if (previous != -1 && utf8proc_grapheme_break_stateful(previous, mystr->current,
    &state)) {

            }
          }

          start--;
          }*/
    // rewind codepoint iterator to beginning of cluster
    mystr->pos_bytes = mystr->cluster_start;
    printf("mystr->pos_bytes:%d\n", mystr->pos_bytes);
    while (MyStr_prev_codepoint(mystr)) {
        // printf("MyStr_prev_cluster0: pos_bytes:%d\n", mystr->pos_bytes);
        // printf("MyStr_prev_cluster1 pos_bytes:%d\n", mystr->pos_bytes);
        int previous = mystr->current;
        utf8proc_int32_t state = 0;
        size_t pos_bytes = mystr->pos_bytes;
        printf("pos_bytes:%d\n", pos_bytes);
        while (MyStr_next_codepoint(mystr)) {
            // printf("MyStr_next_codepoint\n");
            if (previous != -1 &&
                utf8proc_grapheme_break_stateful(previous, mystr->current, &state)) {
                if (mystr->pos_bytes == mystr->cluster_start) {
                    printf("pos_bytes:%d\n", pos_bytes);
                    mystr->pos_bytes = pos_bytes;
                    mystr->cluster_start = mystr->pos_bytes;
                    // MyStr_next_cluster(mystr);
                    return 1;
                } else {
                    break;
                }
            }
        }
        // printf("MyStr_prev_cluster2 pos_bytes:%d\n", mystr->pos_bytes);
        mystr->pos_bytes = pos_bytes;
        // printf("MyStr_prev_cluster3 pos_bytes:%d\n", mystr->pos_bytes);
    }

    return 0;
}

int MyStr_get_grapheme_at_index(MyStr *mystr, int cluster_id) {
    if (cluster_id == mystr->pos_cluster)
        return 1;
    if (cluster_id < mystr->pos_cluster) {
        MyStr_init(mystr, mystr->utf8);
    }
    while (mystr->pos_cluster != cluster_id) {
        if (!MyStr_next_cluster(mystr))
            return 0;
    }
    return 1;
}

int MyStr_get_grapheme_at_idx(MyStr *mystr, int idx) {
    while (mystr->cluster_start < idx) {
        if (!MyStr_next_cluster(mystr))
            return 0;
    }
    return 1;
}

void MyStr_get_last_grapheme(MyStr *self) {
    while (!MyStr_is_end(self)) {
        MyStr_next_cluster(self);
    }
}

int MyStr_get_grapheme_at_column(MyStr *mystr, int column_number) {
    if (mystr->pos_cluster != -1 && mystr->pos_column <= column_number &&
        column_number < mystr->pos_column + mystr->width_column) {
        return 1;
    }
    if (column_number < mystr->pos_column) {
        MyStr_init(mystr, mystr->utf8);
        MyStr_next_cluster(mystr);
    }
    while (column_number > mystr->pos_column) {
        if (!MyStr_next_cluster(mystr)) {
            return 0;
        }
        if (mystr->pos_cluster != -1 && mystr->pos_column <= column_number &&
            column_number < mystr->pos_column + mystr->width_column) {
            return 1;
        }
    }
    if (mystr->pos_cluster != -1 && mystr->pos_column <= column_number &&
        column_number < mystr->pos_column + mystr->width_column)
        return 1;
    return 0;
}

/*
https://github.com/stdlib-js/string-prev-grapheme-cluster-break/blob/main/lib/main.js


static bool is_safe_grapheme_start(unsigned char c)
{
    return c < 0x80 && c != '\r' && c != '\n';
}


size_t candidate = previous_codepoint(s, offset);

while (!is_safe_start(s, candidate))
    candidate = previous_codepoint(s, candidate);

return next_grapheme(s, candidate);

~/Downloads/libunistring-1.4.2/tests/unigbrk/test-u8-grapheme-prev.c
https://fossies.org/linux/libunistring/lib/unigbrk/u-grapheme-prev.h
~/Downloads/libunistring-1.4.2/lib/unigbrk/u-grapheme-prev.h
*/

void print_cluster(MyStr *mystr) {
    printf("cluster_start: %d cluster_size: %d pos_cluster: %d pos_column: %d width_column: %d "
           "width_codepoints: %d ",
           mystr->cluster_start, mystr->cluster_size, mystr->pos_cluster, mystr->pos_column,
           mystr->width_column, mystr->width_codepoints);
    printf("cluster: \"%.*s\"\n", (int)(mystr->cluster_size), mystr->utf8 + mystr->cluster_start);
}

void mystr_test(void) {
    printf(" # test\n");
    //            01234567890
    char *text = "á́ b 👨‍👩‍👧‍👦 👨‍👩‍👧‍👦 🇵🇱 abcd";
    // text = "bc";
    MyStr mystr;
    MyStr_init(&mystr, text);
    while (MyStr_next_cluster(&mystr)) {
        print_cluster(&mystr);
    }

    printf(" # test2\n");
    int cluster_id = 0;
    while (MyStr_get_grapheme_at_index(&mystr, cluster_id)) {
        print_cluster(&mystr);
        cluster_id += 2;
    }

    printf(" # test3\n");
    int column_number = 0;
    while (MyStr_get_grapheme_at_column(&mystr, column_number)) {
        print_cluster(&mystr);
        column_number += 2;
    }
    // test_wc_len();
    printf(" # test4\n");
    if (MyStr_get_grapheme_at_column(&mystr, 8)) {
        print_cluster(&mystr);
    }
}

/*
int main(void) {
    orig();
    test();
}
*/
int MyStr_prev_cluster_fast(MyStr *mystr) {
    const char *terminal = get_terminal_type();

    // Check if we're at the beginning
    if (mystr->pos_cluster <= 0 || mystr->cluster_start == 0)
        return 0;

    // Use wcwidth_grapheme_boundary_before to find the previous cluster start efficiently
    size_t prev_cluster_start = wcwidth_grapheme_boundary_before(
        mystr->utf8, mystr->len_bytes, mystr->cluster_start);

    // Calculate the previous cluster's size
    size_t prev_cluster_end = mystr->cluster_start;
    size_t prev_cluster_size = prev_cluster_end - prev_cluster_start;

    // Set the cluster boundaries
    mystr->cluster_start = prev_cluster_start;
    mystr->cluster_size = prev_cluster_size;
    mystr->pos_bytes = prev_cluster_start;

    // Scan through the previous cluster to extract codepoint information
    mystr->width_codepoints = 0;
    mystr->first_codepoint = -1;

    size_t offset = prev_cluster_start;
    utf8proc_int32_t codepoint;

    while (offset < prev_cluster_end) {
        ssize_t bytes_read = utf8proc_iterate(
            (const utf8proc_uint8_t *)(mystr->utf8 + offset),
            prev_cluster_end - offset, &codepoint);

        if (bytes_read <= 0)
            break;

        if (mystr->first_codepoint == -1)
            mystr->first_codepoint = codepoint;

        mystr->current = codepoint;
        mystr->width_bytes = bytes_read;
        mystr->width_codepoints += 1;
        offset += bytes_read;
    }

    // Calculate display width using wcwidth
    mystr->width_column = wcstwidth_u8(mystr->utf8 + mystr->cluster_start,
                                       mystr->cluster_size, 1, terminal);
    if (mystr->utf8[mystr->cluster_start] == '\t' && mystr->cluster_size == 1)
        mystr->width_column = tab_width;

    // Update position tracking AFTER calculating the new width_column
    mystr->pos_cluster -= 1;
    mystr->pos_column -= mystr->width_column;

    return 1;
}

int MyStr_prev_cluster(MyStr *mystr) {
    MyStr_get_grapheme_at_index(mystr, mystr->pos_cluster - 1);
}
