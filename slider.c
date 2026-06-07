#include <stdio.h>
#include <stdlib.h>
#include "window.h"
#include "slider.h"
#include "logger.h"


void Slider_hover(Window *wg, int x, int y)
{
  Window *slider_grip = ((Slider_data *)wg->data)->slider_grip;
  slider_grip->hidden = 0;
}

void Slider_grip_hover(Window *wg, int x, int y)
{
  wg->hidden = 0;
}

void Slider_grip_undo_hover(Window *wg, int x, int y)
{
  wg->hidden = 1;
}

void Slider_undo_hover(Window *wg, int x, int y)
{
  Window *slider_grip = ((Slider_data *)wg->data)->slider_grip;
  slider_grip->hidden = 1;
}

void Slider_on_mouse_down(Window *wg, int x, int y)
{
  Slider_data *slider_data = (Slider_data *)wg->data;
  Window *child = slider_data->child;
  //LOG_INFO("Slider_on_mouse_down %d %d", slider_data->height, child->calculated.height);
  //child->shift -= child->calculated.height;
  child->shift -= child->calculated.height;
}

void on_mouse_down_slider_grip(Window *wg, int x, int y)
{
  draggingX = NULL;
  draggingY = wg;
  dragging_offset_x = x - wg->parent->left;
  dragging_offset_y = y - wg->parent->top - wg->top;
}

void Slider_set_top(struct Window *w, int top){
  w->top = top;
  Slider_data *slider_data = (Slider_data *)w->parent->data;
  Window *child = slider_data->child;
  int height = child->calculated.height - 1;
  //int height = Window_get_height(w->parent) - 1;
  //LOG_INFO("Slider_set_top %d %d", Window_get_height(w->parent), child->calculated.height);
  child->shift = -w->top * (child->virtual_height - height - 1) / height;
}

Window *slider_new(Window *fm){
  Window *fm_slider = malloc(sizeof *fm_slider);
  Window_init(fm_slider, -1, -1, -1, -1, -1, -1);
  fm_slider->id = "fm_slider";
  Window_append(fm_slider, fm);

  Window *slider = malloc(sizeof *slider);
  //Window_init(slider, -1, 0, 0, 0, 2, -1);
  Window_init(slider, -1, -1, -1, -1, -1, -1);
  slider->left = 0;
  slider->right = 0;
  slider->top = 0;
  slider->bottom = 0;
  //slider->width = 2;
  slider->id = "slider";
  Window_append(fm_slider, slider);
  slider->on_hover = Slider_hover;
  slider->undo_on_hover = Slider_undo_hover;
  slider->on_mouse_down = Slider_on_mouse_down;

  Slider_data *slider_data = (Slider_data *)malloc(sizeof(Slider_data));
  slider->data = slider_data;
  slider_data->child = fm;
  //slider_data->height = height;
  //slider_data->virtual_height = virtual_height;

  //Window *slider_grip = Window_add_widget(slider, -1, 0, 0, -1, 2, 1, "░░", 232, 255);
  Window *slider_grip = Window_add_widget(slider, -1, -1, -1, -1, -1, -1, "░░", 232, 255);
  slider_grip->right = 0;
  slider_grip->top = 0;
  slider_grip->width = 2;
  slider_grip->height = 1;
  slider_grip->on_mouse_down = on_mouse_down_slider_grip;
  slider_grip->hidden = 1;
  slider_grip->on_hover = Slider_grip_hover;
  slider_grip->undo_on_hover = Slider_grip_undo_hover;
  slider_grip->set_top = Slider_set_top;
  slider_data->slider_grip = slider_grip;

  return fm_slider;
}
