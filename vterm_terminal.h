#ifndef VTERM_TERMINAL_H
#define VTERM_TERMINAL_H

#include "window.h"

#ifdef _WIN32
    /* Windows: Include sys/types.h for pid_t, define dummy VTerm types */
    #include <sys/types.h>

    typedef struct { int dummy; } VTerm;
    typedef struct { int dummy; } VTermScreen;
    typedef struct { int dummy; } VTermScreenCell;
    typedef struct { int dummy; } VTermScreenCallbacks;
#else
    #include <vterm.h>
    #include <sys/types.h>
#endif

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

typedef struct TerminalWindow {
    struct Window win;
    char * cwd;
    Window * slider;
    pid_t pid;

    int cursor_x;
    int cursor_y;
    int selection_x;
    int selection_y;

    ScrollbackLine *last_line;
    int last_line_idx;

    /* data formerly in vterm_terminal_data */
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
