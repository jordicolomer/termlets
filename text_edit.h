#ifndef TEXT_EDIT_H
#define TEXT_EDIT_H
#include "window.h"

typedef struct LineEditorWindow {
    struct Window win;
    char buffer[1024];
    int cursor;
} LineEditorWindow;

LineEditorWindow * LineEditorWindow_new(char * c);

#endif
