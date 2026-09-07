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

/*void WM_init(Window *self){
    int selected = (int) self->data;
    int j = 0;

    Tab * tabs = all_tabs_head;
    while (tabs != NULL){
        int bg = 255;
        if (j == selected) bg = SELECTED_COLOR;
        Window_add_widget(self, 0, 0, j++, -1, -1, -1, tabs->child->id, 232, bg);
        tabs = tabs->all_tabs_next;
    }
}*/

void WM_draw(struct Window *w, int hasFocus)
{
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
}

void WM_up(Window *self){
    int selected = (int) self->data;
    self->data = (void*) selected - 1;
}

void WM_down(Window *self){
    int selected = (int) self->data;
    self->data = (void*) selected + 1;
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
    w->draw = WM_draw;
    w->send_key = WM_send_key;
    w->send_sequence = WM_send_sequence;
    vm_frame->focused = w;
    return vm_frame;
}
