#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "window.h"
#include "logger.h"
#include "buffer.h"
#include "utils.h"
#include "lambda.h"

void Window_set_top(struct Window *wg, int top) { wg->top = top; }

Window *root = NULL;

Window *draggingX = NULL;
Window *draggingY = NULL;
Window *resizing = NULL;
Window *focused = NULL;
Window *hovering = NULL;
Window *open_menu = NULL;
int dragging_offset_x, dragging_offset_y;

void Window_append(Window *w, Window *new_w) {
    new_w->parent = w;
    new_w->next = NULL;
    new_w->prev = w->tail;

    if (w->tail != NULL) {
        w->tail->next = new_w;
    } else {
        w->head = new_w;
    }

    w->virtual_height = max(w->virtual_height, new_w->top + new_w->height);

    w->tail = new_w;
}

void Window_remove(Window *w) {
    if (w == NULL || w->parent == NULL) {
        return;
    }

    Window *parent = w->parent;

    /* Link previous node to next node */
    if (w->prev != NULL) {
        w->prev->next = w->next;
    } else {
        /* w was the head */
        parent->head = w->next;
    }

    /* Link next node to previous node */
    if (w->next != NULL) {
        w->next->prev = w->prev;
    } else {
        /* w was the tail */
        parent->tail = w->prev;
    }

    /* Fully detach w */
    w->parent = NULL;
    w->next = NULL;
    w->prev = NULL;
}

int get_bg(struct Window *current, int hasFocus) {
    int framesOverCount = 0;
    Window *cursor = current;
    while (cursor->parent != root) {
        cursor = cursor->parent;
    }
    cursor = cursor->next;
    while (cursor != NULL) {
        framesOverCount += 1;
        cursor = cursor->next;
    }
    int fg = current->fg;
    int bg = current->bg;
    int isTaskBarOrChild = (strcmp(current->id, "taskBar") == 0) ||
                           (current->parent != NULL && current->parent->id != NULL &&
                            strcmp(current->parent->id, "taskBar") == 0);
    if (strcmp(current->id, "menu") != 0 && !isTaskBarOrChild) {
        if (bg >= 232 + 4 && !hasFocus)
            bg -= 2 * framesOverCount;
        if (bg == WINDOW_BAR_COLOR && !hasFocus)
            bg = 243;
    }
    return bg;
}

void Window_fill(struct Window *w, int hasFocus) {
    Geometry geo = w->calculated;
    int fg = w->fg;
    int bg = w->bg;

    bg = get_bg(w, hasFocus);

    for (int i = 0; i < geo.height; i++)
        Buffer_print(&main_buf, geo.y + i, geo.x, geo.width, "", fg, bg);
}

void Window_draw(struct Window *w, int hasFocus) {
    if (w->fill == 1)
        Window_fill(w, hasFocus);
    Geometry geo = w->calculated;
    if (w->hidden == 1) {
        return;
    }

    if (focused == w)
        hasFocus = 1;

    Window *current = w->head;
    int child_count = 0;
    while (current != NULL) {
        child_count++;

        int left = current->left;
        if (left != -1)
            left += w->shift_x;
        int right = current->right;
        int width = current->width;

        if (left == -1)
            left = geo.width - current->right - current->width;
        if (right == -1)
            right = geo.width - current->left - current->width;
        if (width == -1)
            width = geo.width - current->left - current->right;
        if (left < -1) {
            left = geo.width + current->left;
        }
        if (right < -1) {
            right = geo.width + current->right;
        }

        int top = current->top + w->shift;
        int bottom = current->bottom;
        int height = current->height;

        if (top == -1)
            top = geo.height - current->height - current->bottom;
        if (bottom == -1)
            bottom = geo.height - current->top - current->height;
        if (height == -1)
            height = geo.height - current->top - current->bottom;

        if (left + width > geo.width) {
            width = geo.width - left;
        }
        Geometry rect = {geo.x + left, geo.y + top, width, height};
        current->calculated = rect;
        int skip = 0;
        if (geo.height <= top) {
            skip = 1;
        }
        if (height == 1 && top < 0)
            skip = 1;
        if (left < 0)
            skip = 1;
        if (geo.width < left)
            skip = 1;
        if (!skip)
            current->draw(current, hasFocus || current == draggingY);
        current = current->next;
    }
}

Window *Window_init(Window *w, int left, int right, int top, int bottom, int width, int height) {
    w->head = NULL;
    w->tail = NULL;
    w->next = NULL;
    w->prev = NULL;

    w->left = left;
    w->right = right;
    w->top = top;
    w->bottom = bottom;
    w->width = width;
    w->height = height;
    w->virtual_height = height;
    w->draw = Window_draw;
    w->set_top = Window_set_top;
    w->on_mouse_down = NULL;
    w->on_command_mouse_down = NULL;
    w->send_key = NULL;
    w->on_hover = NULL;
    w->undo_on_hover = NULL;
    w->parent = NULL;
    w->hidden = 0;
    w->shift = 0;
    w->shift_x = 0;

    w->c = NULL;

    w->on_mouse_up = NULL;
    w->focused = NULL;

    return w;
}

int Geometry_in_bounds(Geometry geo, int x, int y) {
    if (geo.x <= x && x < geo.x + geo.width && geo.y <= y && y < geo.y + geo.height) {
        return 1;
    }
    return 0;
}

Window *Window_find_widget(struct Window *this, int x, int y) {
    Geometry geo = this->calculated;
    if (!this)
        return NULL;
    if (this->hidden == 1) {
        return NULL;
    }

    Window *ret = NULL;
    if (Geometry_in_bounds(geo, x, y)) {
        ret = this;
    }
    Window *current = this->tail;
    while (current != NULL) {
        int skip = 0;

        /* Skip widgets that are outside parent's visible area (scrolled out of view) */
        if (current->parent) {
            Geometry parent_geo = current->parent->calculated;
            Geometry child_geo = current->calculated;

            // Check if child is completely outside parent's bounds
            if (child_geo.y >= parent_geo.y + parent_geo.height) {
                skip = 1; // below parent's bottom
            }
            if (child_geo.height == 1 && child_geo.y < parent_geo.y) {
                skip = 1; // single-line widget above parent's top
            }
            if (child_geo.y + child_geo.height <= parent_geo.y) {
                skip = 1; // completely above parent's top
            }
        }

        if (!skip) {
            Window *found = Window_find_widget(current, x, y);
            if (found != NULL) {
                return found;
            }
        }

        current = current->prev;
    }
    return ret;
}

void Window_bring_to_bottom(Window *this) {
    LOG_INFO("Window_bring_to_bottom: %p", (void *)this->parent);
    if (!this || !this->parent)
        return;

    Window *parent = this->parent;

    // If already the tail, nothing to do
    if (parent->tail == this)
        return;

    // ---- 1. Unlink from current position ----
    if (this->prev) {
        this->prev->next = this->next;
    } else {
        // this was head
        parent->head = this->next;
    }

    if (this->next) {
        this->next->prev = this->prev;
    } else {
        // this was tail (redundant check, but safe)
        parent->tail = this->prev;
    }

    // ---- 2. Insert at tail ----
    this->next = NULL;
    this->prev = parent->tail;

    if (parent->tail) {
        parent->tail->next = this;
    }

    parent->tail = this;

    // If list was empty or had one element
    if (parent->head == NULL) {
        parent->head = this;
    }
}

void Window_bring_to_top(Window *this) {
    LOG_INFO("Window_bring_to_top: %p", (void *)this->parent);
    if (!this || !this->parent)
        return;

    Window *parent = this->parent;

    // If already the head, nothing to do
    if (parent->head == this)
        return;

    // ---- 1. Unlink from current position ----
    if (this->prev) {
        this->prev->next = this->next;
    } else {
        // this was head (redundant check, but safe)
        parent->head = this->next;
    }

    if (this->next) {
        this->next->prev = this->prev;
    } else {
        // this was tail
        parent->tail = this->prev;
    }

    // ---- 2. Insert at head ----
    this->prev = NULL;
    this->next = parent->head;

    if (parent->head) {
        parent->head->prev = this;
    }

    parent->head = this;

    // If list was empty or had one element
    if (parent->tail == NULL) {
        parent->tail = this;
    }
}

/* widget.c */

void Widget_draw(struct Window *current, int hasFocus) {
    Geometry geo = current->calculated;
    if (current->hidden == 1) {
        return;
    }

    // check if visible
    int visible = 1;
    int framesOverCount = 0;
    Window *cursor = current;
    while (cursor->parent != root) {
        cursor = cursor->parent;
    }
    cursor = cursor->next;
    while (cursor != NULL) {
        if (cursor->left < geo.x && geo.x + geo.width < cursor->left + cursor->width &&
            cursor->top < geo.y && geo.y + geo.height < cursor->top + cursor->height) {
            return;
        }
        framesOverCount += 1;
        cursor = cursor->next;
    }

    int fg = current->fg;
    int bg = current->bg;
    int isTaskBarOrChild = (strcmp(current->id, "taskBar") == 0) ||
                           (current->parent != NULL && current->parent->id != NULL &&
                            strcmp(current->parent->id, "taskBar") == 0);
    if (strcmp(current->id, "menu") != 0 && !isTaskBarOrChild) {
        if (bg >= 232 + 4 && !hasFocus) {
            bg -= 2 * framesOverCount;
            bg = max(bg, 236);
        }
        if (bg == WINDOW_BAR_COLOR && !hasFocus) {
            bg -= framesOverCount;
            bg = max(bg, 16);
        }
    }
#ifdef USE_BUFFER
    Buffer_print(&main_buf, geo.y, geo.x, geo.width, current->c, fg, bg);
#else

    Buffer_print_raw(&main_buf, geo.y, geo.x, geo.width, current->c, fg, bg);
#endif
}

Window *Window_add_widget(Window *w, int left, int right, int top, int bottom, int width,
                          int height, char *c, int fg, int bg) {
    Window *wg = malloc(sizeof *wg);
    Window_init(wg, left, right, top, bottom, width, height);
    wg->draw = Widget_draw;
    wg->parent = w;
    wg->c = c;
    wg->id = c;
    wg->fg = fg;
    wg->bg = bg;

    Window_append(w, wg);

    w->virtual_height = max(w->virtual_height, top + height);

    return wg;
}

void Window_execute_lambda(struct Window *w, int x, int y) { invoke_lambda(w->lambda); }

Window *Window_get_frame(struct Window *w) {
    Window *current = w;
    while (current->parent != NULL) {
        LOG_INFO("Window_get_frame %p %s", current, current->id);
        current = current->parent;
        if (current->parent == root)
            return current;
    }
    return current;
}
