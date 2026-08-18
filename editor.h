#ifndef FILE_EDITOR_H
#define FILE_EDITOR_H
#include "window.h"
#include "tabs.h"
#include "text_edit.h"

typedef struct Node {
    char *line;
    struct Node *next;
    struct Node *prev;
    size_t length;   // characters in memory, excluding '\0'
    size_t width;   // width on screen
    size_t capacity; // allocated bytes
    int lexerState; // used for syntax highlighting
} Node;

typedef struct EditorPointer {
    int n;
    int x;
    int ptr;
} EditorPointer;

typedef struct EditorWindow {
    struct Window win;
    Window *slider;
    Node *head;
    Node *tail;
    //Node *top; // pointer to top_n. todo: remove this
    int top_n; // what line is shown as first line
    //Node *cursor;
  
    struct EditorPointer cursor;
    struct EditorPointer selection;
    struct EditorPointer highlight_start;
    struct EditorPointer highlight_end;
  /*int cursor_n; // what line has the cursor
    int cursor_x; // what column has the cursor
    int cursor_ptr; // pointer to cursor location in memory
  
    int selection_n; // what line has the selection marker. -1 means no marker
    int selection_x; // what column has the selection marker
    int selection_ptr; // pointer to cursor location in memory*/
  
    int n_lines; // total number of lines
    char * file_path;
    int language; // specifies the syntax highlighting language
} EditorWindow;

typedef struct EditorFrame {
    struct Window win;
    Tabs *tabs;
    LineEditorWindow *search_box;
    //Window *syntax;
} EditorFrame;

Window *Editor_new(int left, int right, int top, int bottom, int width, int height);
void Editor_open_file(EditorFrame * editor_frame, char * file_path);
void Editor_last_open_file(char * file_path);

#endif
