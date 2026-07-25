#ifndef VTERM_TERMINAL_H
#define VTERM_TERMINAL_H
#include "window.h"

typedef struct TerminalWindow {
    struct Window win;
    char * cwd;
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
