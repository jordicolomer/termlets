#include <stdlib.h>
#include "frame.h"
#include "logger.h"
#include "taskbar.h"

void on_mouse_down_window_bar(Widget *wg, int x, int y)
{
  LOG_INFO("on_mouse_down_window_bar");
  dragging = wg->parent;
  focused = wg->parent;
  Window_bring_to_bottom(dragging);
  dragging_offset_x = x - wg->parent->left;
  dragging_offset_y = y - wg->parent->top;
  TaskBar_switch((Widget *)(wg->parent->data));
}

void on_mouse_down_close(Widget *wg, int x, int y)
{
  LOG_INFO("on_mouse_down_close");
  Window_remove(wg->parent);
  focused = NULL;
}

void Window_add_window_bar(struct Window *w)
{
  Widget *wg = Window_add_widget(w, 0, 0, 0, -1, -1, 1, "", 255, WINDOW_BAR_COLOR);
  wg->on_mouse_down = on_mouse_down_window_bar;

  Widget *close = Window_add_widget(w, -1, 1, 0, -1, 1, 1, "X", 255, WINDOW_BAR_COLOR);
  close->on_mouse_down = on_mouse_down_close;

  Widget *maximize = Window_add_widget(w, -1, 3, 0, -1, 1, 1, "□", 255, WINDOW_BAR_COLOR);
  Widget *minimize = Window_add_widget(w, -1, 5, 0, -1, 1, 1, "-", 255, WINDOW_BAR_COLOR);
}

void Widget_on_resize(Widget *wg, int x, int y)
{
  LOG_INFO("Widget_on_resize");

  resizing = wg->parent;
  dragging_offset_x = x - wg->parent->width;
  dragging_offset_y = y - wg->parent->height;
}

Window *Frame_init(Window *w, int left, int right, int top, int bottom, int width, int height)
{
  Window_init(w, left, right, top, bottom, width, height);
  Window_add_window_bar(w);

  Window *child = malloc(sizeof *child);
  // Window_init(child, 1, 1, width-2, height-2);
  Window_init(child, 0, 0, 1, 0, -1, -1);
  Window_append(w, child);
  child->id = "child";

  Widget *resize_grip = Window_add_widget(w, -1, 0, -1, 0, 1, 1, "⌟", 232, 255);
  resize_grip->on_mouse_down = Widget_on_resize;

  return child;
}
