#ifndef VTERM_TERMINAL_H
#define VTERM_TERMINAL_H
#include "window.h"

Window *VTermTerminal_new(int left, int right, int top, int bottom, int width, int height);

/* PTY monitoring thread functions */
void start_pty_monitor_thread();
void stop_pty_monitor_thread();
int check_and_clear_repaint_flag();

#endif
