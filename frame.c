#include <stdlib.h>
#include "frame.h"
#include "logger.h"
#include "taskbar.h"

void on_mouse_down_window_bar(Window *wg, int x, int y)
{
  LOG_INFO("on_mouse_down_window_bar");
  draggingX = wg->parent;
  draggingY = wg->parent;
  focused = wg->parent;
  Window_bring_to_bottom(draggingY);
  dragging_offset_x = x - wg->parent->left;
  dragging_offset_y = y - wg->parent->top;
  TaskBar_switch((Window *)(wg->parent->data));
}

void on_mouse_down_close(Window *wg, int x, int y)
{
  LOG_INFO("on_mouse_down_close");
  Window_remove(wg->parent);
  focused = NULL;
}

void Window_add_window_bar(struct Window *w)
{
  Window *wg = Window_add_widget(w, 0, 0, 0, -1, -1, 1, "", 255, WINDOW_BAR_COLOR);
  wg->id = "window bar";
  wg->on_mouse_down = on_mouse_down_window_bar;

  Window *close = Window_add_widget(w, -1, 1, 0, -1, 1, 1, "X", 255, WINDOW_BAR_COLOR);
  close->on_mouse_down = on_mouse_down_close;

  Window *maximize = Window_add_widget(w, -1, 3, 0, -1, 1, 1, "□", 255, WINDOW_BAR_COLOR);
  Window *minimize = Window_add_widget(w, -1, 5, 0, -1, 1, 1, "-", 255, WINDOW_BAR_COLOR);
}

void Widget_on_resize(Window *wg, int x, int y)
{
  LOG_INFO("Widget_on_resize");

  resizing = wg->parent;
  dragging_offset_x = x - wg->parent->width;
  dragging_offset_y = y - wg->parent->height;
}

Window *Frame_init(Window *w, int left, int right, int top, int bottom, int width, int height, Window *child, int black_grip)
{
  Window_init(w, left, right, top, bottom, width, height);
  Window_add_window_bar(w);

  if (child == NULL){
    child = malloc(sizeof *child);
    Window_init(child, 0, 0, 1, 0, -1, -1);
    child->id = "child";
  }
  Window_append(w, child);

  int fg = 232;
  int bg = 255;
  if (black_grip){
    fg = 255;
    bg = 234;
  }
  Window *resize_grip = Window_add_widget(w, -1, 0, -1, 0, 1, 1, "J", fg, bg);
  resize_grip->on_mouse_down = Widget_on_resize;

  return child;
}
