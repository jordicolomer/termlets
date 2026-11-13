#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "window.h"
#include "slider.h"
#include "logger.h"
#include "buffer.h"
#include "utils.h"

/* --- Class ScrollGrip --- */

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

void update_width(Window *slider_grip, Window *fm) {
    if (fm->virtual_height == 0) {
        slider_grip->width = 1;
        slider_grip->hidden = 1;
        return;
    }
    slider_grip->width = fm->calculated.width * fm->calculated.width / fm->virtual_width;
    if (slider_grip->width >= fm->calculated.width)
        slider_grip->hidden = 1;
    slider_grip->width = min(slider_grip->width, fm->calculated.width);
    slider_grip->width = max(slider_grip->width, 1);
}

void Slider_grip_hover(Window *wg, int x, int y) { wg->hidden = 0; }

void Slider_grip_undo_hover(Window *wg, int x, int y) { wg->hidden = 1; }

void on_mouse_down_slider_grip(Window *wg, int x, int y) {
    LOG_INFO("on_mouse_down_slider_grip");
    ScrollGrip * self = wg;
    if (self->is_vertical){
        draggingY = wg;
        dragging_offset_y = y - wg->parent->top - wg->top;
    } else {
        LOG_INFO("on_mouse_down_slider_grip else");
        draggingX = wg;
        dragging_offset_x = x - wg->parent->left - wg->left;
    }
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

void Slider_set_left(struct Window *w, int left) {
    w->left = left;
    ScrollView * scroll = w->parent->parent;
    Window *child = scroll->child;
    // how much free room there is for the handle
    int width = child->calculated.width - scroll->horizontal_bar->slider_grip->width;
    if (width == 0) {
        child->shift_x = 0;
        return;
    }
    child->shift_x = -w->left * (child->virtual_width - child->calculated.width) / width;
}

Window * Slider_grip_new(int is_vertical) {
    ScrollGrip *slider_grip = my_malloc(sizeof *slider_grip);
    slider_grip->is_vertical = is_vertical;
    Window_init(slider_grip, -1, -1, -1, -1, -1, -1);
    slider_grip->win.bg = 250;
    slider_grip->win.fg = 0;
    slider_grip->win.fill = 1;
    slider_grip->win.id = "slider grip";
    if (is_vertical) {
        slider_grip->win.right = 0;
        slider_grip->win.top = 0;
        slider_grip->win.width = 2;
        slider_grip->win.height = 1;
    } else {
        slider_grip->win.left = 0;
        slider_grip->win.top = 0;
        slider_grip->win.width = 4;
        slider_grip->win.height = 1;
    }
    slider_grip->win.on_mouse_down = on_mouse_down_slider_grip;
    slider_grip->win.hidden = 1;
    slider_grip->win.on_hover = Slider_grip_hover;
    slider_grip->win.undo_on_hover = Slider_grip_undo_hover;
    slider_grip->win.set_top = Slider_set_top;
    slider_grip->win.set_left = Slider_set_left;
    return slider_grip;
}






/* --- Class ScrollBar --- */

void Slider_hover(Window *w, int x, int y) {
    ScrollBar * scrollBar = w;
    ScrollView * scroll = w->parent;
    scroll->vertical_bar->slider_grip->hidden = 0;
    scroll->horizontal_bar->slider_grip->hidden = 0;

    if (scrollBar->is_vertical) update_height(scroll->vertical_bar->slider_grip, scroll->child);
    else update_width(scroll->horizontal_bar->slider_grip, scroll->child);
}

void Slider_undo_hover(Window *w, int x, int y) {
    ScrollView * scroll = w->parent;
    scroll->vertical_bar->slider_grip->hidden = 1;
    scroll->horizontal_bar->slider_grip->hidden = 1;
}

// update grip position based on shift
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

ScrollBar * Slider_new(int is_vertical) {
    ScrollBar *slider = my_malloc(sizeof *slider);
    slider->is_vertical = is_vertical;
    Window_init(slider, -1, -1, -1, -1, -1, -1);
    if (is_vertical) {
        slider->win.right = 0;
        slider->win.top = 0;
        slider->win.bottom = 0;
        slider->win.width = 2;
    } else {
        slider->win.left = 0;
        slider->win.right = 0;
        slider->win.bottom = 0;
        slider->win.height = 1;
    }
    slider->win.id = "slider";
    slider->win.on_hover = Slider_hover;
    slider->win.undo_on_hover = Slider_undo_hover;
    slider->win.draw = Slider_draw;

    Window *slider_grip = Slider_grip_new(is_vertical);
    Window_append(slider, slider_grip);
    slider->slider_grip = slider_grip;

    return slider;
}





/* --- Class ScrollView --- */

Window *ScrollView_new(Window *fm) {
    ScrollView * scroll_view = my_malloc(sizeof *scroll_view);
    Window_init(scroll_view, -1, -1, -1, -1, -1, -1);
    scroll_view->child = fm;
    scroll_view->win.id = "fm_slider";
    scroll_view->win.window_type = 'sldr';
    Window_append(scroll_view, fm);

    scroll_view->vertical_bar = Slider_new(1);
    Window_append(scroll_view, scroll_view->vertical_bar);

    scroll_view->horizontal_bar = Slider_new(0);
    Window_append(scroll_view, scroll_view->horizontal_bar);

    scroll_view->win.focused = fm;

    return scroll_view;
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