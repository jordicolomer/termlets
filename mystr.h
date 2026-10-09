#ifndef MYSTR_H
#define MYSTR_H
#include <utf8proc.h>

typedef struct MyStr {
    char *utf8;                // utf8 byte array
    size_t len_bytes;          // total lenght in bytes
    size_t width_column;       // width in columns of the current cluster
    size_t width_codepoints;   // width in codepoints of the current cluster
    utf8proc_int32_t first_codepoint;   // first codepoint of the current cluster
    size_t pos_column;         // offset in column number of the current cluster
    size_t pos_bytes;          // offset in bytes of the current cluster
    size_t pos_cluster;        // ordinal number of the current cluster
    size_t cluster_start;      // offset in bytes for the start of the current cluster
    size_t cluster_end;        // offset in bytes for the end of the current cluster
    utf8proc_int32_t current;  // unicode code point
    utf8proc_int32_t previous; // previous unicode code point
    utf8proc_int32_t state;    // state used for grapheme break detection
} MyStr;

void MyStr_init(MyStr *mystr, char *utf8);
int MyStr_next_codepoint(MyStr *mystr);
int MyStr_next_cluster(MyStr *mystr);
int MyStr_get_grapheme_at_index(MyStr *mystr, int cluster_id);
int MyStr_get_grapheme_at_column(MyStr *mystr, int column_number);
int wc_len(char *text, int length);

#endif
