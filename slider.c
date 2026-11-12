#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "window.h"
#include "slider.h"
#include "logger.h"
#include "buffer.h"
#include "utils.h"

/* --- BEGIN: Slider grip --- */

// update grip size based on child size
void update_height(Window *slider_grip, Window *fm) {
    if (fm->virtual_height == 0) {
        slider_grip->height = 1;
        slider_grip->hidden = 1;
        return;
    }
    slider_grip->height = fm->calculated.height * fm->calculated.height / fm->virtual_height;
    if (slider_grip->height >= fm->calculated.height)
        slider_grip->hidden = 1;
    slider_grip->height = min(slider_grip->height, fm->calculated.height);
    slider_grip->height = max(slider_grip->height, 1);
}

void Slider_grip_hover(Window *wg, int x, int y) { wg->hidden = 0; }

void Slider_grip_undo_hover(Window *wg, int x, int y) { wg->hidden = 1; }

void on_mouse_down_slider_grip(Window *wg, int x, int y) {
    draggingX = NULL;
    draggingY = wg;
    dragging_offset_x = x - wg->parent->left;
    dragging_offset_y = y - wg->parent->top - wg->top;
}

// update child shift based on grip position
void Slider_set_top(struct Window *w, int top) {
    w->top = top;
    ScrollView * scroll = w->parent->parent;
    Window *child = scroll->child;
    // how much free room there is for the handle
    int height = child->calculated.height - scroll->vertical_bar->slider_grip->height;
    if (height == 0) {
        child->shift = 0;
        return;
    }
    child->shift = -w->top * (child->virtual_height - child->calculated.height) / height;
}

void slider_grip_draw(struct Window *w, int hasFocus) {
    if (w->hidden == 1)
        return;
    Geometry geo = w->calculated;
    for (int i = 0; i < w->height; i++)
        Buffer_print(&main_buf, geo.y + i, geo.x, geo.width, "  ", 0, 250);
}

Window * Slider_grip_new() {
    Window *slider_grip = my_malloc(sizeof *slider_grip);
    Window_init(slider_grip, -1, -1, -1, -1, -1, -1);
    slider_grip->draw = slider_grip_draw;
    slider_grip->right = 0;
    slider_grip->top = 0;
    slider_grip->width = 2;
    slider_grip->height = 1;
    slider_grip->on_mouse_down = on_mouse_down_slider_grip;
    slider_grip->hidden = 1;
    slider_grip->on_hover = Slider_grip_hover;
    slider_grip->undo_on_hover = Slider_grip_undo_hover;
    slider_grip->set_top = Slider_set_top;
    return slider_grip;
}

/* --- END: Slider grip --- */






/* --- BEGIN: Slider --- */

void Slider_hover(Window *w, int x, int y) {
    ScrollView * scroll = w->parent;
    scroll->vertical_bar->slider_grip->hidden = 0;
    update_height(scroll->vertical_bar->slider_grip, scroll->child);
}

void Slider_undo_hover(Window *w, int x, int y) {
    ScrollView * scroll = w->parent;
    scroll->vertical_bar->slider_grip->hidden = 1;
}

void Slider_update_top(struct Window *w) {
    ScrollView * scroll = w->parent;
    Window *child = scroll->child;
    update_height(scroll->vertical_bar->slider_grip, child);
    int height = child->calculated.height - scroll->vertical_bar->slider_grip->height;
    int denominator = child->virtual_height - child->calculated.height;
    if (denominator == 0) {
        scroll->vertical_bar->slider_grip->top = 0;
        return;
    }
    scroll->vertical_bar->slider_grip->top = -child->shift * height / denominator;
}

void Slider_draw(struct Window *current, int hasFocus) {
    Slider_update_top(current);
    Window_draw(current, hasFocus);
}

ScrollBar * Slider_new() {
    ScrollBar *slider = my_malloc(sizeof *slider);
    Window_init(slider, -1, -1, -1, -1, -1, -1);
    slider->win.left = -1;
    slider->win.right = 0;
    slider->win.top = 0;
    slider->win.bottom = 0;
    slider->win.width = 2;
    slider->win.id = "slider";
    slider->win.on_hover = Slider_hover;
    slider->win.undo_on_hover = Slider_undo_hover;
    slider->win.draw = Slider_draw;

    Window *slider_grip = Slider_grip_new();
    Window_append(slider, slider_grip);
    slider->slider_grip = slider_grip;

    return slider;
}


/* --- END: Slider --- */





/* --- BEGIN: ScrollView --- */

Window *ScrollView_new(Window *fm) {
    ScrollView *fm_slider = my_malloc(sizeof *fm_slider);
    Window_init(fm_slider, -1, -1, -1, -1, -1, -1);
    fm_slider->child = fm;
    fm_slider->win.id = "fm_slider";
    fm_slider->win.window_type = 'sldr';
    Window_append(fm_slider, fm);

    ScrollBar *slider = Slider_new();
    fm_slider->vertical_bar = slider;
    Window_append(fm_slider, slider);

    fm_slider->win.focused = fm;

    return fm_slider;
}

void Slider_make_visible(ScrollView *self, Window *wg) {
    if (self == NULL)
        return;
    Window *child = self->child;

    int efective_top = wg->top + child->shift;
    int is_visible = 0 <= efective_top && efective_top <= child->calculated.height - 1;
    if (is_visible)
        return;

    child->shift = -(max(0, wg->top - child->calculated.height / 2));
    int min_shift = -(child->virtual_height - child->calculated.height);
    child->shift = max(child->shift, min_shift);
}

void Slider_show_grip(ScrollView *self) {
    if (self == NULL)
        return;
    self->vertical_bar->slider_grip->hidden = 0;
}

void Slider_scroll_up(struct ScrollView * self) {
    if (self == NULL)
        return;
    self->child->shift = min(0, self->child->shift + 1);
    Slider_show_grip(self);
}

void Slider_scroll_down(struct ScrollView * self) {
    if (self == NULL)
        return;
    if (self->child->virtual_height <= self->child->calculated.height)
        return;
    self->child->shift--;
    int min_shift = -(self->child->virtual_height - self->child->calculated.height);
    self->child->shift = max(self->child->shift, min_shift);
    Slider_show_grip(self);
}

void Slider_reset(ScrollView *self) {
    Slider_set_top(self->vertical_bar->slider_grip, 0);
}

/* --- END: ScrollView --- */
