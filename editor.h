#ifndef FILE_EDITOR_H
#define FILE_EDITOR_H
#include "window.h"

typedef struct Node {
    char *line;
    struct Node *next;
    struct Node *prev;
} Node;

typedef struct EditorWindow {
    struct Window win;
    Node *head;
    Node *tail;
    Node *top;
    int top_n;
    //Node *cursor;
    int cursor_n;
    int n_lines;
} EditorWindow;

typedef struct EditorFrame {
    struct Window win;
    Window *tabs;
} EditorFrame;

Window *Editor_new(int left, int right, int top, int bottom, int width, int height);
void Editor_open_file(EditorFrame * editor_frame, char * file_path);
void Editor_last_open_file(char * file_path);

#endif
