#ifndef FILE_EDITOR_H
#define FILE_EDITOR_H
#include "window.h"
#include "tabs.h"
#include "mini_edit.h"

typedef struct Node {
    char *line;
    struct Node *next;
    struct Node *prev;
    size_t length;   // characters in memory, excluding '\0'
    size_t width;    // width on screen
    size_t capacity; // allocated bytes
    int lexerState;  // used for syntax highlighting
} Node;

typedef struct EditorPointer {
    int y;
    int x;
    int ptr;
} EditorPointer;

typedef struct EditorWindow {
    struct Window win;
    struct Tab tab;
    Window *slider;
    Node *head;
    Node *tail;
    int top_n; // what line is shown as first line

    struct EditorPointer cursor;
    struct EditorPointer selection;
    struct EditorPointer highlight_start;
    struct EditorPointer highlight_end;

    int n_lines; // total number of lines
    char *file_path;
    int language; // specifies the syntax highlighting language
    int selecting;
    LineEditorWindow *search_box;
} EditorWindow;

typedef struct EditorFrame {
    struct Window win;
    Tabs *tabs;
} EditorFrame;

Window *Editor_new(int left, int right, int top, int bottom, int width, int height);
void Editor_open_file(EditorFrame *editor_frame, char *file_path);
void Editor_last_open_file(char *file_path);
void EditorFrame_search(EditorFrame *self);

#endif
