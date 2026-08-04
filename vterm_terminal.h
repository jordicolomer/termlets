#ifndef VTERM_TERMINAL_H
#define VTERM_TERMINAL_H
#include <vterm.h>
#include "window.h"

typedef struct ScrollbackLine {
    VTermScreenCell *cells;
    int cols;
    struct ScrollbackLine *prev;
    struct ScrollbackLine *next;
} ScrollbackLine;

typedef struct ScrollbackList {
    ScrollbackLine *head;
    ScrollbackLine *tail;
    int count;
    int max_size;
} ScrollbackList;

/*typedef struct vterm_terminal_data {
    int master;
    VTerm *vt;
    VTermScreen *vts;
    Window *terminal;
    int rows;
    int cols;
    ScrollbackList scrollback;
    VTermScreenCallbacks callbacks;
} vterm_terminal_data;*/

typedef struct TerminalWindow {
    struct Window win;
    char * cwd;
    Window * slider;
    pid_t pid;
    int edit_mode;

    int cursor_x;
    int cursor_y;
    int selection_x;
    int selection_y;

    ScrollbackLine *last_line;
    int last_line_idx;

    // data formely in vterm_terminal_data
    int master;
    VTerm *vt;
    VTermScreen *vts;
    Window *terminal;
    int rows;
    int cols;
    ScrollbackList scrollback;
    VTermScreenCallbacks callbacks;
} TerminalWindow;

typedef struct TerminalFrame {
    struct Window win;
    Window *tabs;
} TerminalFrame;

Window *VTermTerminal_new(int left, int right, int top, int bottom, int width, int height);
TerminalWindow *VTermTerminal_window(int initial_rows, int initial_cols);

/* PTY monitoring thread functions */
void start_pty_monitor_thread();
void stop_pty_monitor_thread();
int check_and_clear_repaint_flag();

#endif
