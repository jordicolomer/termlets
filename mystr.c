#include <stdio.h>
#include <string.h>
#include <utf8proc.h>
#include "uwidth.h"
#include "mystr.h"
#include "logger.h"
#include "common.h"

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

        if (length == 1) {
            if (current == 9633) return 1;
            if (current == 9475) return 1;
            if (current == 0x2192) return 1;
            if (current == 0x25B2) return 1;
            if (current == 0x25BC) return 1;
            if (current == 0x1F3DE) return 1;
            if (current == 0x2193) return 1;
            if (current == '\t') return tab_width;
			if (current >= 0x0300 && current <= 0x036F) return 0;
			if (current <= 31 && current != '\t') return 0;

        }

        uwidth_push(&state, current, &event);
        pos += bytes;
    }
    uwidth_finish(&state, &event);
    //LOG_INFO("wc_len %s %d %d", text, length, event.width);
    return event.width;
}

void test_wc_len() {
    char *text = " a";
    printf("space len %d\n", wc_len(text, 1));
}

void MyStr_init(MyStr *mystr, char *utf8) {
    mystr->utf8 = utf8;
    mystr->len_bytes = strlen(utf8);
    mystr->width_column = 0;
    mystr->pos_bytes = 0;
    mystr->pos_column = 0;
    mystr->pos_cluster = -1;

    mystr->cluster_end = 0;
    mystr->cluster_start = 0;
    mystr->previous = -1;
    mystr->current = -1;
    mystr->state = 0;
}

int MyStr_next_codepoint(MyStr *mystr) {
    if (!(mystr->pos_bytes < mystr->len_bytes))
        return 0;
    mystr->previous = mystr->current;
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

int MyStr_next_cluster(MyStr *mystr) {
    if (mystr->cluster_end == mystr->len_bytes)
        return 0;
    mystr->cluster_start = mystr->cluster_end;
    mystr->pos_column += mystr->width_column;
    mystr->width_codepoints = 0;
    if (mystr->pos_cluster == -1)
        mystr->width_codepoints = -1;
    // size_t pos_column = mystr->pos_column;
    size_t pos = mystr->pos_bytes;
    while (MyStr_next_codepoint(mystr)) {
        // printf("U+%04X\n", (unsigned int)mystr->current);
        mystr->width_codepoints += 1;
        if (mystr->previous != -1 &&
            utf8proc_grapheme_break_stateful(mystr->previous, mystr->current, &mystr->state)) {
            mystr->cluster_end = pos;
            mystr->pos_cluster += 1;
            // printf("%d %d\n", mystr->cluster_start, mystr->cluster_end);
            // LOG_INFO("MyStr_next_cluster %s %d", mystr->utf8, mystr->width_codepoints);
            int w = wc_len(mystr->utf8 + mystr->cluster_start, mystr->width_codepoints);
            // mystr->pos_column += w;
            mystr->width_column = w;
            // mystr->width_codepoints-=1;
            return 1;
        }
        pos = mystr->pos_bytes;
    }
    mystr->width_codepoints += 1;
    mystr->pos_cluster += 1;
    mystr->cluster_end = mystr->len_bytes;
    // int w = wc_len(mystr->utf8 + mystr->cluster_start, mystr->cluster_end -
    // mystr->cluster_start);
    int w = wc_len(mystr->utf8 + mystr->cluster_start, mystr->width_codepoints);
    // mystr->pos_column += w;
    mystr->width_column = w;
    return 1;
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
    printf("cluster_start: %d cluster_end: %d pos_cluster: %d pos_column: %d width_column: %d "
           "width_codepoints: %d ",
           mystr->cluster_start, mystr->cluster_end, mystr->pos_cluster, mystr->pos_column,
           mystr->width_column, mystr->width_codepoints);
    printf("cluster: \"%.*s\"\n", (int)(mystr->cluster_end - mystr->cluster_start),
           mystr->utf8 + mystr->cluster_start);
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
