#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

typedef struct Slider_data
{
  Window *slider_grip;
  Window *child;
  int height;
  int virtual_height;
} Slider_data;

void Slider_hover(Window *wg, int x, int y);
void Slider_grip_hover(Window *wg, int x, int y);
void Slider_grip_undo_hover(Window *wg, int x, int y);
void Slider_undo_hover(Window *wg, int x, int y);
void Slider_on_mouse_down(Window *wg, int x, int y);
void on_mouse_down_slider_grip(Window *wg, int x, int y);
void Slider_set_top(struct Window *w, int top);
Window *FileExplorer_new(int left, int right, int top, int bottom, int width, int height);
Window *FileExplorer_test(int x, int y, int width, int height);

#endif