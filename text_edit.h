#ifndef TEXT_EDIT_H
#define TEXT_EDIT_H
#include <stdlib.h>
#include "window.h"

typedef struct LineEditorWindow {
    struct Window win;
    char * empty_label;
    char buffer[1024];
    int cursor;
} LineEditorWindow;

LineEditorWindow * LineEditorWindow_new(char * c, char * empty_label);

void insert_char(char *buffer, size_t pos, char c, 
                 size_t current_len, size_t capacity);
void delete_char(char *buffer, size_t pos, size_t len);

#endif
