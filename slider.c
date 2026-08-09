#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "window.h"
#include "slider.h"
#include "logger.h"
#include "buffer.h"
#include "utils.h"

void Slider_reset(Window *wg){
  Window *slider_grip = ((Slider_data *)wg->data)->slider_grip;
  //slider_grip->top = 0;
  Slider_set_top(slider_grip, 0);
}

void update_height(Window *slider_grip, Window *fm){
  if (fm->virtual_height == 0) {
    slider_grip->height = 1;
    slider_grip->hidden = 1;
    return;
  }
  slider_grip->height = fm->calculated.height * fm->calculated.height / fm->virtual_height;
  if (slider_grip->height >= fm->calculated.height) slider_grip->hidden = 1;
  slider_grip->height = min(slider_grip->height, fm->calculated.height);
  slider_grip->height = max(slider_grip->height, 1);
  //LOG_INFO("update_height slider_grip->height %d", slider_grip->height);
  //LOG_INFO("update_height fm->calculated.height %d", fm->calculated.height);
  //LOG_INFO("update_height fm->virtual_height %d", fm->virtual_height);
}

void Slider_hover(Window *wg, int x, int y)
{
  //LOG_INFO("Slider_hover");
  Window *slider_grip = ((Slider_data *)wg->data)->slider_grip;
  slider_grip->hidden = 0;
  Window *fm = ((Slider_data *)wg->data)->child;
  update_height(slider_grip, fm);
  //slider_grip->height = fm->calculated.height * fm->calculated.height / fm->virtual_height;
  //slider_grip->height = min(slider_grip->height, fm->calculated.height);
  //LOG_INFO("slider_new %d %d %d %d", fm->height, fm->height, fm->virtual_height, slider_grip->height);

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
  // how much free room there is for the handle
  int height = child->calculated.height - slider_data->slider_grip->height;
  // child->virtual_height - child->calculated.height is the first visible element when the scroll is at the bottom
  // w->top / height is the fraction of the scroll (0 top 1 bottom)
  if (height == 0) {
    child->shift = 0;
    return;
  }
  child->shift = -w->top * (child->virtual_height - child->calculated.height) / height;
}

void Slider_update_top(struct Window *w){
  // invert Slider_set_top
  Slider_data *slider_data = (Slider_data *)w->data;
  //LOG_INFO("Slider_update_top %p", slider_data);
  Window *child = slider_data->child;
  update_height(slider_data->slider_grip, child);
  int height = child->calculated.height - slider_data->slider_grip->height;
  //LOG_INFO("Slider_update_top height %d %d %d", child->calculated.height, slider_data->slider_grip->height, slider_data->slider_grip->shift);
  //LOG_INFO("pre Slider_update_top %d %d %d %d", slider_data->slider_grip->top, child->shift, (child->virtual_height - child->calculated.height), height);
  //LOG_INFO("Slider_update_top slider_data->slider_grip->top %d", slider_data->slider_grip->top);
  //LOG_INFO("Slider_update_top child->shift %d", child->shift);
  //LOG_INFO("Slider_update_top height %d", height);
  //LOG_INFO("Slider_update_top (child->virtual_height - child->calculated.height) %d",(child->virtual_height - child->calculated.height));
  //LOG_INFO("Slider_update_top fraction child->shift / (child->virtual_height - child->calculated.height) %d",child->shift / (child->virtual_height - child->calculated.height));
  int denominator = child->virtual_height - child->calculated.height;
  if (denominator == 0) {
    slider_data->slider_grip->top = 0;
    return;
  }
  slider_data->slider_grip->top = - child->shift * height / denominator;
  //LOG_INFO("post Slider_update_top %d", slider_data->slider_grip->top);
}

void slider_grip_draw(struct Window *w, int hasFocus){
  if (w->hidden == 1) return;
  Geometry geo = w->calculated;
  for (int i=0;i<w->height;i++)
    Buffer_print(&main_buf, geo.y+i, geo.x, geo.width, "  ", 0, 250);
}

void Slider_draw(struct Window *current, int hasFocus){
  Slider_update_top(current);
  Window_draw(current, hasFocus);
}

Window *slider_new(Window *fm){
  Window *fm_slider = malloc(sizeof *fm_slider);
  Window_init(fm_slider, -1, -1, -1, -1, -1, -1);
  fm_slider->id = "fm_slider";
  Window_append(fm_slider, fm);

  Window *slider = malloc(sizeof *slider);
  //Window_init(slider, -1, 0, 0, 0, 2, -1);
  Window_init(slider, -1, -1, -1, -1, -1, -1);
  slider->left = -1;
  slider->right = 0;
  slider->top = 0;
  slider->bottom = 0;
  slider->width = 2;
  slider->id = "slider";
  Window_append(fm_slider, slider);
  //Window_append(slider, fm);
  slider->on_hover = Slider_hover;
  slider->undo_on_hover = Slider_undo_hover;
  //slider->on_mouse_down = Slider_on_mouse_down;
  slider->draw = Slider_draw;

  Slider_data *slider_data = (Slider_data *)malloc(sizeof(Slider_data));
  memset(slider_data, 0, sizeof(Slider_data));  // Zero-initialize to prevent garbage values
  slider->data = slider_data;
  fm_slider->data = slider_data;
  slider_data->child = fm;
  //slider_data->height = height;
  //slider_data->virtual_height = virtual_height;

  //Window *slider_grip = Window_add_widget(slider, -1, 0, 0, -1, 2, 1, "░░", 232, 255);
  //Window *slider_grip = Window_add_widget(slider, -1, -1, -1, -1, -1, -1, "░░", 232, 255);
  Window *slider_grip = malloc(sizeof *slider_grip);
  Window_init(slider_grip, -1, -1, -1, -1, -1, -1);
  slider_grip->parent = slider;
  Window_append(slider, slider_grip);

  slider_grip->draw = slider_grip_draw;
  slider_grip->right = 0;
  slider_grip->top = 0;
  slider_grip->width = 2;
  slider_grip->height = 1;
  //fm->virtual_height / fm->calculated.height = fm->calculated.height / slider_grip->height
  slider_grip->on_mouse_down = on_mouse_down_slider_grip;
  slider_grip->hidden = 1;
  slider_grip->on_hover = Slider_grip_hover;
  slider_grip->undo_on_hover = Slider_grip_undo_hover;
  slider_grip->set_top = Slider_set_top;
  slider_data->slider_grip = slider_grip;

  fm_slider->focused = fm;

  return fm_slider;
}

void Slider_make_visible(Window *w, Window * wg){
  if (w == NULL) return;
  //LOG_INFO("Slider_make_visible %p %p", w, wg);
  Slider_data * slider_data = w->data;
  Window *child = slider_data->child;

  int efective_top = wg->top + child->shift;
  int is_visible = 0 <= efective_top && efective_top <= child->calculated.height - 1;
  if (is_visible) return;

  //LOG_INFO("Slider_make_visible2 %d %d", wg->top, child->height);
  child->shift = -(max(0, wg->top - child->calculated.height/2));
  int min_shift = -(child->virtual_height - child->calculated.height);
  child->shift = max(child->shift, min_shift);
  //LOG_INFO("Slider_make_visible2 %p %d %d", slider_data->slider_grip, wg->top, child->shift);
  //Slider_set_top(slider_data->slider_grip, wg->top);
}

void Slider_show_grip(Window *w){
  if (w == NULL) return;
  Slider_data * slider_data = w->data;
  slider_data->slider_grip->hidden = 0;
}

void Slider_scroll_up(struct Window *w){
  if (w == NULL) return;
  Slider_data * slider_data = w->data;
  Window *child = slider_data->child;
  child->shift = min(0, child->shift+1);
  Slider_show_grip(w);
}

void Slider_scroll_down(struct Window *w){
  if (w == NULL) return;
  Slider_data * slider_data = w->data;
  Window *child = slider_data->child;
  child->shift--;
  int min_shift = -(child->virtual_height - child->calculated.height);
  child->shift = max(child->shift, min_shift);
  Slider_show_grip(w);
}
