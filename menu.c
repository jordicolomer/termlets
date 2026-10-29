#ifdef _WIN32
/* Disable strict pointer type warnings on Windows for this file */
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <stdarg.h>

/* Provide asprintf for Windows */
static int asprintf(char **strp, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = _vscprintf(fmt, args);
    if (len < 0) {
        va_end(args);
        return -1;
    }
    *strp = malloc(len + 1);
    if (!*strp) {
        va_end(args);
        return -1;
    }
    int result = vsprintf(*strp, fmt, args);
    va_end(args);
    return result;
}
#endif

#include "menu.h"
#include "utils.h"
#include "logger.h"
#include "buffer.h"
#include "lambda.h"
#include "common.h"

void Execute_lambda(struct Window *w, int x, int y) { invoke_lambda(w->lambda); }

Menu *auto_expand;

void Execute_lambda_and_close_parent(struct Window *w, int x, int y) {
    Window *submenu = w->parent;
    Menu *menu = submenu->parent;
    // LOG_INFO("Execute_lambda_and_close_parent: %s %p", menu->win.id, menu);
    // menu->auto_expand = 0;
    // Window *menu = w->parent;
    submenu->hidden = 1;
    invoke_lambda(w->lambda);
    auto_expand = NULL;
}

Menu *Menu_create_vertical(Window *parent) {
    Menu *menu = malloc(sizeof *menu);
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    menu->offset = 0;
    menu->submenu = NULL;
    menu->vertical = 0;

    menu->win.left = 0;
    menu->win.top = 0;
    menu->win.width = 0;
    menu->win.height = 0;
    menu->vertical = 1;
    menu->win.hidden = 1;

    Window_append(parent, menu);

    // background
    Window_add_widget(menu, 0, 0, 0, -1, -1, 1, "", 232, 253);

    return menu;
}

Menu *Menu_create_horizontal() {
    Menu *menu = malloc(sizeof *menu);
    menu->offset = 0;
    menu->submenu = NULL;
    menu->vertical = 0;
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    menu->win.left = 0;
    menu->win.right = 0;
    menu->win.top = 0;
    menu->win.height = 1;

    // background
    Window_add_widget(menu, 0, 0, 0, -1, -1, 1, "", 232, 253);

    return menu;
}

void Menu_move_to_button(Window *menu, Window *button) {
    Window *self = menu->parent;
    menu->left = button->calculated.x - self->calculated.x;
    menu->top = button->calculated.y - self->calculated.y + 1;
}

Menu *last_opened_submenu = NULL;

void Menu_close_last_opened_submenu() {
    if (last_opened_submenu != NULL)
        last_opened_submenu->win.hidden = 1;
}

void Menu_toggle_auto_expand(Menu *self) {
    if (auto_expand == NULL) {
        auto_expand = self;
    } else {
        auto_expand = NULL;
        Menu_close_last_opened_submenu();
    }
}

/*void Menu_mark_selected(Window *win) {
    if (win->lambda == NULL)
        return;
    Window *current = win->head;
    int selected = invoke_lambda(win->lambda);
    if (selected == 0)
        return;
    int i = 0;
    while (current != NULL) {
        if (current->c != NULL && strlen(current->c) > 1) {
            current->c[1] = ' ';
            if (selected == i) {
                current->c[1] = 'x';
            }
        }
        current = current->next;
        i++;
    }
	}*/

void Menu_show_submenu(Menu *menu, Window *win) {
    Menu_close_last_opened_submenu();
    last_opened_submenu = menu;
    menu->win.hidden = 0;
    Menu_move_to_button(menu, win);
    //Menu_mark_selected(menu);
}

void Menu_toggle_submenu(Menu *menu, Window *win) {
    menu->win.hidden = 1 - menu->win.hidden;
    Menu_move_to_button(menu, win);
    //Menu_mark_selected(menu);
}

void Menu_change_color_hover_and_open(Window *w, int x, int y) {
    w->fg = 232;
    w->bg = 27;
    Menu *self = w;
    Menu *parent = w->parent;
    if (auto_expand == parent) {
        Menu_show_submenu(w->data, w);
    }
}

void Menu_change_color_hover(Window *w, int x, int y) {
    w->fg = 232;
    w->bg = 27;
}

void Menu_change_color_normal(Window *w, int x, int y) {
    w->fg = 232;
    w->bg = 253;
}

void Menu_change_color_darker(Window *w, int x, int y) {
    w->fg = 232;
    w->bg = 248;
}

void Menu_change_color_selected(Window *w, int x, int y) {
    w->fg = 232;
    w->bg = SELECTED_COLOR;
}

void Menu_draw(struct Window *current, int hasFocus) {
  Menu *self = current;
  if (self->show_check_lambda == NULL){
	Widget_draw(current, hasFocus);
	return;
  }
    if (current->hidden == 1) {
        return;
    }
  Geometry geo = current->calculated;
  int fg = current->fg;
  int bg = current->bg;
  int x_shift = 0;
  if (self->show_check_lambda != NULL) x_shift = 3;
  Buffer_print(&main_buf, geo.y, geo.x, x_shift, "", fg, bg);
  //Buffer_print(&main_buf, geo.y, geo.x+x_shift, geo.width-x_shift, current->c, fg, bg);
  Buffer_print_dots(&main_buf, geo.y, geo.x+x_shift, geo.width-x_shift, current->c, fg, bg);
  //LOG_INFO("Menu_draw %s", current->c);

  if (self->show_check_lambda != NULL && invoke_lambda(self->show_check_lambda)){
	//LOG_INFO("show_check");
	char * check_str = "✓";
	Buffer_print(&main_buf, geo.y, geo.x+1, 1, check_str, fg, bg);
  }
}

Menu *Menu_add_menu(Window *w, int left, int right, int top, int bottom, int width,
                          int height, char *c, int fg, int bg) {
    Menu *wg = malloc(sizeof *wg);
  //Window *wg = malloc(sizeof *wg);
    Window_init(wg, left, right, top, bottom, width, height);
    wg->win.draw = Menu_draw;
    wg->win.parent = w;
    wg->win.c = c;
    wg->win.id = c;
    wg->win.fg = fg;
    wg->win.bg = bg;
	wg->show_check_lambda = NULL;

    Window_append(w, wg);

    w->virtual_height = max(w->virtual_height, top + height);

    return wg;
}

Menu *Menu_add_element(Menu *self, char *name, Lambda *lambda) {
    int len = strlen(name);
    Menu *win = NULL;
    if (self->vertical == 1) {
        win = Menu_add_menu(self, 0, 0, self->offset, -1, -1, 1, name, 232, 248);
        self->win.width = max(self->win.width, len + 3);
        self->win.height += 1;
        self->offset += 1;
        if (lambda != NULL) {
            win->win.on_hover = Menu_change_color_hover;
            win->win.undo_on_hover = Menu_change_color_darker;
            win->win.lambda = lambda;
            win->win.on_mouse_down = Execute_lambda_and_close_parent;
        }
    } else {
        win = Menu_add_menu(self, self->offset, -1, 0, -1, len, 1, name, 232, 253);
        win->win.on_hover = Menu_change_color_hover_and_open;
        win->win.undo_on_hover = Menu_change_color_normal;
        self->offset += len;
        if (lambda != NULL) {
            win->win.lambda = lambda;
            win->win.on_mouse_down = Execute_lambda;
        }
    }
    return win;
}

void Menu_list_windows_item_selected(Tab *tab, Window *menu) {
    tab_select(tab);
    menu->hidden = 1;
    auto_expand = NULL;
}

int return_true(){
  return 1;
}

int return_false(){
  return 0;
}

Window *Menu_list_windows(Menu *menu, Tabs *tabs, struct Window *window_menu_item, Menu *submenu) {
  //Menu_toggle_auto_expand(menu);
  // if (menu != auto_expand)
  //    return NULL;

    submenu->win.head = NULL;
    submenu->win.tail = NULL;
    submenu->offset = 0;
    submenu->win.width = 0;

    Tab *tab = tabs->first;
    int i = 0;
    while (tab != NULL) {
	  //char *selected_sign = "   ";

	  //Window *win = Menu_add_element(submenu, tab->str, NULL);
	  Window *win = Menu_add_element(submenu, tab->short_str, NULL);
        win->undo_on_hover = Menu_change_color_darker;
        if (tabs->selected_tab == tab) {
		  //win->bg = SELECTED_COLOR;
		  //win->undo_on_hover = Menu_change_color_selected;
		  Menu * menu = win;
		  menu->show_check_lambda = create_lambda(return_true, 0);
        } else {
		  Menu * menu = win;
		  menu->show_check_lambda = create_lambda(return_false, 0);
		}
        win->on_hover = Menu_change_color_hover;
        win->lambda = create_lambda(Menu_list_windows_item_selected, 2, tab, submenu);
        win->on_mouse_down = Execute_lambda;

        tab = tab->next;
    }
    Menu_add_element(submenu, "", NULL);

    Menu_show_submenu(submenu, window_menu_item);
    return (Window *)submenu;
}

Window *Menu_list_toggle_windows(Menu *menu, Tabs *tabs, struct Window *window_menu_item, Menu *submenu) {
    Menu_toggle_auto_expand(menu);
    if (menu != auto_expand)
        return NULL;
	return Menu_list_windows(menu, tabs, window_menu_item, submenu);
}

void Menu_windows_hover(Window *w, int x, int y) {
    w->fg = 232;
    w->bg = 27;
    Menu *self = w;
    Menu *parent = w->parent;
    if (auto_expand == parent) {
	  invoke_lambda(w->data3);
    }
}

void Menu_add_windows(Menu *menu, char *name, Tabs *tabs, Window *self) {
    int len = strlen(name);
    menu->offset += 1;
    Window *window_menu_item =
        Window_add_widget(menu, menu->offset, -1, 0, -1, len, 1, name, 232, 253);
    window_menu_item->on_hover = Menu_windows_hover;
    window_menu_item->undo_on_hover = Menu_change_color_normal;
    menu->offset += len + 1;

    Menu *submenu = Menu_create_vertical(self);
    window_menu_item->lambda =
        create_lambda(Menu_list_toggle_windows, 4, menu, tabs, window_menu_item, submenu);
    window_menu_item->data3 =
        create_lambda(Menu_list_windows, 4, menu, tabs, window_menu_item, submenu);
    window_menu_item->on_mouse_down = Execute_lambda;

    window_menu_item->data = submenu;
}

void Menu_add_submenu(Menu *self, char *name, Menu *menu) {
    Window *win = Menu_add_element(self, name, NULL);
    win->data = menu;
    win->lambda = create_lambda(Menu_toggle_auto_expand, 1, self);
    win->on_mouse_down = Execute_lambda;
}
