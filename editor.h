#ifndef FILE_EDITOR_H
#define FILE_EDITOR_H
#include "window.h"
#include "tabs.h"

typedef struct Node {
    char *line;
    struct Node *next;
    struct Node *prev;
    size_t length;   // characters, excluding '\0'
    size_t capacity; // allocated bytes
} Node;

typedef struct EditorWindow {
    struct Window win;
    Window *slider;
    Node *head;
    Node *tail;
    //Node *top; // pointer to top_n. todo: remove this
    int top_n; // what line is shown as first line
    //Node *cursor;
    int cursor_n; // what line has the cursor
    int cursor_x; // what column has the cursor
    int n_lines; // total number of lines
    int edit_mode;
} EditorWindow;

typedef struct EditorFrame {
    struct Window win;
    Window *tabs;
} EditorFrame;

Window *Editor_new(int left, int right, int top, int bottom, int width, int height);
void Editor_open_file(EditorFrame * editor_frame, char * file_path);
void Editor_last_open_file(char * file_path);

#endif
