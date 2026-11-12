#ifndef SLIDER_H
#define SLIDER_H

typedef struct ScrollView {
    Window win;
    Window *slider_grip;
    Window *child;
} ScrollView;


void Slider_hover(Window *wg, int x, int y);
void Slider_grip_hover(Window *wg, int x, int y);
void Slider_grip_undo_hover(Window *wg, int x, int y);
void Slider_undo_hover(Window *wg, int x, int y);
//void Slider_on_mouse_down(Window *wg, int x, int y);
void on_mouse_down_slider_grip(Window *wg, int x, int y);
void Slider_set_top(struct Window *w, int top);
void Slider_update_top(struct Window *w);
Window *slider_new(Window *fm);
void Slider_reset(ScrollView *wg);
void Slider_make_visible(ScrollView *w, Window *wg);
void Slider_show_grip(ScrollView *self);
void Slider_scroll_up(struct ScrollView *self);
void Slider_scroll_down(struct ScrollView *self);

#endif