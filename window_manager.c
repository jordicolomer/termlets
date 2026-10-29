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
#include "utils.h"
#include "mini_edit.h"
#include "clipboard.h"

void WM_draw(struct Window *w, int hasFocus) {
  WM * self = w;
    if (w->fill == 1)
        Window_fill(w, hasFocus);
    Geometry geo = w->calculated;
    int j = 0;
    Tab *tabs = all_tabs_head;

    while (tabs != NULL) {
	  if (contains_ignore_case(tabs->str, self->line_edit->buffer)) {
        int bg = 255;
        if (j == self->selected){
            bg = SELECTED_COLOR;
			self->selected_tab = tabs;
		}
		Buffer_print_dots(&main_buf, geo.y + j++, geo.x, geo.width, tabs->str, 232, bg);
	  }
	  tabs = tabs->all_tabs_next;
    }
	self->count = j;
}

void WM_up(WM * self) {
  self->selected = max(self->selected - 1, 0);
}

void WM_down(WM * self) {
  self->selected = min(self->selected + 1, self->count-1);
}

void WM_select(WM *self) {
    select_tab(self->selected_tab, 1);
	self->win.parent->hidden = 1;
	Window_bring_to_top(self->win.parent);
	self->selected = 1;

	LineEditorWindow_reset(self->line_edit);
}

void WM_search(WM *self) {
  Window * parent = self->win.parent;
  parent->focused = self->line_edit;
}

void WM_copy(WM *self) {
  clipboard_copy(self->selected_tab->str);
}

void WM_send_key(Window *self, char c) {
    Action action = get_action(c, WT_WINDOW_MANAGER);

    if (action == ACTION_COPY) {
        WM_copy(self);
        return;
    }
    if (action == ACTION_SEARCH) {
        WM_search(self);
        return;
    }
    if (action == ACTION_DOWN) {
        WM_down(self);
        return;
    }
    if (action == ACTION_UP) {
        WM_up(self);
        return;
    }
    if (action == ACTION_WINDOW_MANAGER || action == ACTION_ENTER) {
        WM_select(self);
        return;
    }
}

void WM_send_sequence(Window *self, const char *seq, int len) {
    if (strlen(seq) == 0) {
        insert_mode = 0;
        return;
    }
    if (strcmp(seq, "[A") == 0) {
        WM_up(self);
        return;
    }
    if (strcmp(seq, "[B") == 0) {
        WM_down(self);
        return;
    }
}

void wm_searchbox_exit(WM *self) {
  Window * parent = self->win.parent;
  parent->focused = self;
  self->selected = 0;
  //
}


Window *WM_create(int left, int right, int top, int bottom, int width, int height) {
    Window *vm_frame = malloc(sizeof *vm_frame);
    memset(vm_frame, 0, sizeof *vm_frame); // Zero-initialize to prevent garbage values
	WM *child = malloc(sizeof *child);
	
	LineEditorWindow *line_edit = LineEditorWindow_new(NULL, "Search");
	line_edit->win.top = 1;
    line_edit->win.bottom = -1;
    line_edit->win.left = 0;
    line_edit->win.right = 0;
    line_edit->win.width = -1;
    line_edit->win.height = 1;
    line_edit->win.bg = 229;
    line_edit->win.fg = 16;
    line_edit->win.hidden = 0;
    line_edit->win.id = "WM_searchbox";
	line_edit->on_exit = create_lambda(wm_searchbox_exit, 1, child);
    line_edit->win.lambda = create_lambda(wm_searchbox_exit, 1, child);

	child->line_edit = line_edit;
	
	Window_init(child, 0, 0, 2, 0, -1, -1);
    Window *w = Frame_init(vm_frame, left, right, top, bottom, width, height, child, 0);
    w->id = "wm child";
    w->bg = 255;
    w->fill = 1;
    w->draw = WM_draw;
    w->send_key = WM_send_key;
    w->send_sequence = WM_send_sequence;
	child->selected = 1;
    vm_frame->focused = w;
	
	Window_append(vm_frame, line_edit);
	
    return vm_frame;
}

Window *wm = NULL;

Window *WM_show() {
    if (wm == NULL) {
        int width = root->width / 3;
        int height = root->height / 2;
        wm = WM_create((root->width - width) / 2, -1, (root->height - height) / 2, -1, width,
                       height);
        wm->parent = root;
        wm->id = "WindowManager";
        Window_append(root, wm);
    }
    wm->hidden = 0;
    focused = wm;

    Window_bring_to_bottom(wm);


    return wm;
}
