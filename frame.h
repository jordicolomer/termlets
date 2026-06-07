#ifndef FRAME_H
#define FRAME_H

#include "window.h"

void on_mouse_down_window_bar(Window *wg, int x, int y);
void Window_add_window_bar(struct Window *w);
void Widget_on_resize(Window *wg, int x, int y);
Window *Frame_init(Window *w, int left, int right, int top, int bottom, int width, int height, Window *child);

#endif