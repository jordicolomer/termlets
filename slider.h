#ifndef SLIDER_H
#define SLIDER_H


typedef struct Slider_data
{
  Window *slider_grip;
  Window *child;
  int height;
  int virtual_height;
} Slider_data;

/*
typedef struct Slider_data
{
  Window *slider_grip;
  Window *child;
  int height;
  int virtual_height;
} Slider_data;

*/

void Slider_hover(Window *wg, int x, int y);
void Slider_grip_hover(Window *wg, int x, int y);
void Slider_grip_undo_hover(Window *wg, int x, int y);
void Slider_undo_hover(Window *wg, int x, int y);
void Slider_on_mouse_down(Window *wg, int x, int y);
void on_mouse_down_slider_grip(Window *wg, int x, int y);
void Slider_set_top(struct Window *w, int top);
Window *slider_new(Window *fm, int width, int height, int virtual_height);

#endif
