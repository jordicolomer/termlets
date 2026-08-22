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

void Execute_lambda(struct Window *w, int x, int y)
{
    invoke_lambda(w->lambda);
}

Menu * auto_expand;

void Execute_lambda_and_close_parent(struct Window *w, int x, int y)
{
    Window *submenu = w->parent;
    Menu *menu = submenu->parent;
    //LOG_INFO("Execute_lambda_and_close_parent: %s %p", menu->win.id, menu);
    //menu->auto_expand = 0;
    //Window *menu = w->parent;
    submenu->hidden = 1;
    invoke_lambda(w->lambda);
    auto_expand = NULL;
}


Menu *Menu_create_vertical(Window *parent)
{
    Menu *menu = malloc(sizeof *menu);
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    //menu->auto_expand = 0;
    menu->offset = 0;
    menu->submenu = NULL;
    menu->vertical = 0;

    menu->win.left = 0;
    menu->win.top = 0;
    menu->win.width = 0;
    menu->win.height = 0;
    //menu->parent = parent;
    //menu->self = self;
    menu->vertical = 1;
    menu->win.hidden = 1;

    Window_append(parent, menu);

    // background
    Window_add_widget(menu, 0, 0, 0, -1, -1, 1, "", 232, 253);

    return menu;
}

Menu *Menu_create_horizontal()
{
    Menu *menu = malloc(sizeof *menu);
    //menu->auto_expand = 0;
    menu->offset = 0;
    menu->submenu = NULL;
    menu->vertical = 0;
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    menu->win.left = 0;
    menu->win.right = 0;
    menu->win.top = 0;
    //menu->win.bottom = 0;
    menu->win.height = 1;
    //menu->win.bg = 76;
    //menu->win.fill = 1;
    //menu->parent = parent;

    // background
    Window_add_widget(menu, 0, 0, 0, -1, -1, 1, "", 232, 253);

    return menu;
}

void Menu_move_to_button(Window *menu, Window *button)
{
    Window *self = menu->parent;
    menu->left = button->calculated.x - self->calculated.x;
    menu->top = button->calculated.y - self->calculated.y + 1;
}

Menu * last_opened_submenu = NULL;

void Menu_close_last_opened_submenu()
{
    if (last_opened_submenu != NULL) last_opened_submenu->win.hidden = 1;
}

void Menu_toggle_auto_expand(Menu *self)
{
    //self->auto_expand = 1 - self->auto_expand;
    if (auto_expand == NULL){
        auto_expand = self;
    } else {
        auto_expand = NULL;
        Menu_close_last_opened_submenu();
    }
    //if (self->auto_expand == 0) 
}

void Menu_mark_selected(Window *win){
    if (win->lambda == NULL) return;
    Window * current = win->head;
    int selected = invoke_lambda(win->lambda);
    //int selected = win->int_data;
    if (selected == 0) return;
    int i = 0;
    while(current != NULL){
        if (current->c != NULL && strlen(current->c) > 1){
            current->c[1] = ' ';
            if (selected == i) {
                //memcpy(&current->c[1], "✔", 3);
                current->c[1] = 'x';
            }
        }
        current = current->next;
        i++;
    }
}

void Menu_show_submenu(Menu *menu, Window *win)
{
    Menu_close_last_opened_submenu();
    last_opened_submenu = menu;
    menu->win.hidden = 0;
    Menu_move_to_button(menu, win);
    Menu_mark_selected(menu);
}

void Menu_toggle_submenu(Menu *menu, Window *win)
{
    menu->win.hidden = 1 - menu->win.hidden;
    Menu_move_to_button(menu, win);
    Menu_mark_selected(menu);
}

void Menu_change_color_hover_and_open(Window *w, int x, int y)
{
    w->fg = 232;
    w->bg = 27;
    Menu * self = w;
    Menu * parent = w->parent;
    if (auto_expand == parent){
        Menu_show_submenu(w->data, w);
    }
}

void Menu_change_color_hover(Window *w, int x, int y)
{
    w->fg = 232;
    w->bg = 27;
    //Menu menu = (Menu) wg;
    //invoke_lambda(w->lambda);
    //menu.
}

void Menu_change_color_normal(Window *w, int x, int y)
{
    w->fg = 232;
    w->bg = 253;
}

/*void Menu_lambda(struct Window *w, int x, int y)
{
    Window *(*fn)(Window *, Tabs *, struct Window *) = (Window * (*)(Window *, Tabs *, struct Window *)) w->data;
    Window *self = w->data2;
    Window *tabs = w->data3;
    fn(self, tabs, w);
}

void Menu_lambda1(struct Window *w, int x, int y)
{
    Window *menu = w->data3;
    menu->hidden = 1;

    Window *(*fn)(Window *) = (Window * (*)(Window *)) w->data;
    Window *self = w->data2;
    fn(self);
}*/

Window *Menu_add_element(Menu *self, char *name, Lambda * lambda)
{
    //int len = strlen(name);
    int len = calculate_width(name);
    Window * win = NULL;
    if (self->vertical == 1)
    {
        win = Window_add_widget(self, 0, 0, self->offset, -1, -1, 1, name, 232, 253);
        self->win.width = max(self->win.width, len+2);
        self->win.height += 1;
        self->offset += 1;
        if (lambda != NULL){
            win->on_hover = Menu_change_color_hover;
            win->undo_on_hover = Menu_change_color_normal;
            /*if (self->vertical == 1){
                win->on_hover = Menu_change_color_hover_and_open;
            }*/

            win->lambda = lambda;
            win->on_mouse_down = Execute_lambda_and_close_parent;
        }
    }
    else
    {
        //self->offset += 1;
        win = Window_add_widget(self, self->offset, -1, 0, -1, len, 1, name, 232, 253);
        //win->on_hover = Menu_change_color_hover;
        win->on_hover = Menu_change_color_hover_and_open;
        win->undo_on_hover = Menu_change_color_normal;
        //self->offset += len + 1;
        self->offset += len;
        if (lambda != NULL){
            win->lambda = lambda;
            win->on_mouse_down = Execute_lambda;
        }
    }
    return win;
}


void Menu_list_windows_item_selected(Tab *tab, Window *menu)
{
    tab_select(tab);
    menu->hidden = 1;
    auto_expand = NULL;
}

Window *Menu_list_windows(Menu * menu, Tabs *tabs, struct Window *window_menu_item, Menu *submenu)
{
    Menu_toggle_auto_expand(menu);
    if (menu != auto_expand) return NULL;

    submenu->win.head = NULL;
    submenu->win.tail = NULL;
    submenu->offset = 0;

    Tab *tab = tabs->first;
    int i = 0;
    int maxLen = 0;
    while (tab != NULL)
    {
        char *tab_label = NULL;
        char *selected_sign = " ";
        if (tabs->selected_tab == tab)
            selected_sign = "*";
		int len = strlen(tab->child->id);
		int max_len = 40;
		if (max_len < len){
		  char * shortened = tab->child->id + strlen(tab->child->id) - max_len;
		  asprintf(&tab_label, " %s...%s  ", selected_sign, shortened);
		} else {
		  asprintf(&tab_label, " %s%s  ", selected_sign, tab->child->id);
		}

        Window *win = Menu_add_element(submenu, tab_label, NULL);
        win->on_hover = Menu_change_color_hover;
        win->undo_on_hover = Menu_change_color_normal;
        win->lambda = create_lambda(Menu_list_windows_item_selected, 2, tab, submenu);
        win->on_mouse_down = Execute_lambda;

        maxLen = max(maxLen, strlen(tab_label));
        tab = tab->next;
    }
    Menu_add_element(submenu, "", NULL);

    Menu_show_submenu(submenu, window_menu_item);
    return (Window *)submenu;
}

void Menu_add_windows(Menu *menu, char *name, Tabs *tabs, Window * self)
{
    int len = strlen(name);
    menu->offset += 1;
    Window *window_menu_item = Window_add_widget(menu, menu->offset, -1, 0, -1, len, 1, name, 232, 253);
    //window_menu_item->on_hover = Menu_change_color_hover;
    window_menu_item->on_hover = Menu_change_color_hover_and_open;
    window_menu_item->undo_on_hover = Menu_change_color_normal;
    menu->offset += len + 1;

    Menu *submenu = Menu_create_vertical(self);
    window_menu_item->lambda = create_lambda(Menu_list_windows, 4, menu, tabs, window_menu_item, submenu);
    //window_menu_item->lambda = create_lambda(Menu_toggle_auto_expand, 1, menu);
    window_menu_item->on_mouse_down = Execute_lambda;

    window_menu_item->data = submenu;
}

void Menu_add_submenu(Menu *self, char *name, Menu *menu)
{
    Window *win = Menu_add_element(self, name, NULL);
    //self->submenu = menu;
    win->data = menu;
    //win->lambda = create_lambda(Menu_show_submenu, 2, menu, win);
    win->lambda = create_lambda(Menu_toggle_auto_expand, 1, self);
    win->on_mouse_down = Execute_lambda;
}
