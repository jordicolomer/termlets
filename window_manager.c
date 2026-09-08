#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#include "common.h"
#include "window_manager.h"
#include "window.h"
#include "frame.h"
#include "tabs.h"
#include "buffer.h"
#include "config.h"
#include "logger.h"

void WM_draw(struct Window *w, int hasFocus)
{
    if (w->fill == 1) Window_fill(w, hasFocus);
    Geometry geo = w->calculated;
    int j = 0;
    int selected = (int) w->data;
    Tab * tabs = all_tabs_head;

    while (tabs != NULL){
        int bg = 255;
        if (j == selected) bg = SELECTED_COLOR;
        Buffer_print(&main_buf, geo.y + j++, geo.x, geo.width, tabs->child->id, 232, bg);
        tabs = tabs->all_tabs_next;
    }

    //w->height = j;
}

void WM_up(Window *self){
    int selected = (int) self->data;
    self->data = (void*) selected - 1;
}

void WM_down(Window *self){
    int selected = (int) self->data;
    self->data = (void*) selected + 1;
}

void WM_select(Window *self){
    int selected = (int) self->data;
    int j = 0;
    Tab * tabs = all_tabs_head;
    while (tabs != NULL){
        int bg = 255;
        if (j == selected){
            select_tab(tabs, 1);
            self->parent->hidden = 1;
            Window_bring_to_top(self->parent);
            self->data = (void*) 1;
            return;
        }
        tabs = tabs->all_tabs_next;
        j++;
    }
}

void WM_send_key(Window *self, char c)
{
    Action action = get_mapping()[c];

    if (action == ACTION_DOWN){
	    WM_down(self);
	    return;
    }
    if (action == ACTION_UP){
	    WM_up(self);
	    return;
    }
    if (action == ACTION_WINDOW_MANAGER){
	    WM_select(self);
	    return;
    }
}

void WM_send_sequence(Window *self, const char *seq, int len){
  if (strlen(seq) == 0){ insert_mode = 0; return; }
  if (strcmp(seq, "[A") == 0){ WM_up(self); return; }
  if (strcmp(seq, "[B") == 0){ WM_down(self); return; }
}

Window * WM_create(int left, int right, int top, int bottom, int width, int height){
    Window *vm_frame = malloc(sizeof *vm_frame);
    memset(vm_frame, 0, sizeof *vm_frame);  // Zero-initialize to prevent garbage values
    Window *w = Frame_init(vm_frame, left, right, top, bottom, width, height, NULL, 0);
    w->bg = 255;
    w->fill = 1;
    w->draw = WM_draw;
    w->send_key = WM_send_key;
    w->send_sequence = WM_send_sequence;
    vm_frame->focused = w;
    return vm_frame;
}

Window * wm = NULL;

Window * WM_show(){
    //LOG_INFO("WM_show");
    if (wm == NULL){
        wm = WM_create(20, -1, 20, -1, 90, 20);
        wm->parent = root;
        wm->id = "WindowManager";
        wm->data = (void*) 1;
        Window_append(root, wm);
    }
    wm->hidden = 0;
    focused = wm;

    Window_bring_to_bottom(wm);

    return wm;
}