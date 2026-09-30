#include <stdio.h>
#include <string.h>
#include <utf8proc.h>
#include "uwidth.h"

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
		//printf("U+%04X\n", (unsigned int)current);

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
    size_t len = strlen(text);
    size_t pos = 0;
    for (index = 0U; index < length; ++index) {
        // while (pos < len) {
        utf8proc_int32_t current;
        utf8proc_ssize_t bytes =
            utf8proc_iterate((const utf8proc_uint8_t *)text + pos, len - pos, &current);

        uwidth_push(&state, current, &event);
        pos += bytes;
    }
    uwidth_finish(&state, &event);
    return event.width;
}

void test_wc_len() {
    char *text = " a";
    printf("space len %d\n", wc_len(text, 1));
}

typedef struct MyStr {
    char *utf8;                // utf8 byte array
    size_t len_bytes;          // total lenght in bytes
    size_t width_column;       // width in columns of the current cluster
    size_t width_codepoints;   // width in codepoints of the current cluster
    size_t pos_column;         // offset in column number of the current cluster
    size_t pos_bytes;          // offset in bytes of the current cluster
    size_t pos_cluster;        // ordinal number of the current cluster
    size_t cluster_start;      // offset in bytes for the start of the current cluster
    size_t cluster_end;        // offset in bytes for the end of the current cluster
    utf8proc_int32_t current;  // unicode code point
    utf8proc_int32_t previous; // previous unicode code point
    utf8proc_int32_t state;    // state used for grapheme break detection
} MyStr;

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
    if (!(mystr->pos_bytes < mystr->len_bytes)) return 0;
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
    //return (mystr->pos_bytes < mystr->len_bytes);
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
        //printf("U+%04X\n", (unsigned int)mystr->current);
		mystr->width_codepoints+=1;
        if (mystr->previous != -1 &&
            utf8proc_grapheme_break_stateful(mystr->previous, mystr->current, &mystr->state)) {
            mystr->cluster_end = pos;
            mystr->pos_cluster += 1;
            // printf("%d %d\n", mystr->cluster_start, mystr->cluster_end);
            int w = wc_len(mystr->utf8 + mystr->cluster_start,
                           mystr->cluster_end - mystr->cluster_start);
            // mystr->pos_column += w;
            mystr->width_column = w;
			//mystr->width_codepoints-=1;
            return 1;
        }
        pos = mystr->pos_bytes;
    }
	mystr->width_codepoints+=1;
    mystr->pos_cluster += 1;
    mystr->cluster_end = mystr->len_bytes;
    int w = wc_len(mystr->utf8 + mystr->cluster_start, mystr->cluster_end - mystr->cluster_start);
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

void print_cluster(MyStr *mystr) {
    printf("cluster_start: %d cluster_end: %d pos_cluster: %d pos_column: %d width_column: %d width_codepoints: %d ",
           mystr->cluster_start, mystr->cluster_end, mystr->pos_cluster, mystr->pos_column,
           mystr->width_column, mystr->width_codepoints);
    printf("cluster: \"%.*s\"\n", (int)(mystr->cluster_end - mystr->cluster_start),
           mystr->utf8 + mystr->cluster_start);
}

void test(void) {
    printf(" # test\n");
    //            01234567890
    char *text = "á́ b 👨‍👩‍👧‍👦 👨‍👩‍👧‍👦 🇵🇱 abcd";
    //text = "bc";
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

int main(void) {
    orig();
    test();
}
