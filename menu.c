#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "utils.h"
#include "logger.h"

Menu *Menu_create_vertical(Window *self, Window *parent)
{
    Menu *menu = malloc(sizeof *menu);
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    menu->win.left = 0;
    menu->win.top = 0;
    menu->win.width = 0;
    menu->win.height = 0;
    menu->parent = parent;
    menu->self = self;
    menu->vertical = 1;
    menu->win.hidden = 1;

    Window_append(parent, menu);

    // background
    Window_add_widget(menu, 0, 0, 0, -1, -1, 1, "", 232, 253);

    return menu;
}

Menu *Menu_create_horizontal(Window *parent)
{
    Menu *menu = malloc(sizeof *menu);
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    menu->win.left = 0;
    menu->win.right = 0;
    menu->win.top = 0;
    menu->win.bottom = 0;
    //menu->win.height = 1;
    menu->parent = parent;

    // background
    Window_add_widget(menu, 0, 0, 0, -1, -1, 1, "", 232, 253);

    return menu;
}

void Menu_change_color_hover(Window *wg, int x, int y)
{
    wg->fg = 232;
    wg->bg = 27;
}

void Menu_change_color_normal(Window *wg, int x, int y)
{
    wg->fg = 232;
    wg->bg = 253;
}

void Menu_lambda(struct Window *w, int x, int y)
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
}

Window *Menu_add_element(Menu *self, char *name, void *callback)
{
    int len = strlen(name);
    Window * win = NULL;
    if (self->vertical == 1)
    {
        win = Window_add_widget(self, 0, 0, self->offset, -1, -1, 1, name, 232, 253);
        self->win.width = max(self->win.width, len+2);
        self->win.height += 1;
        self->offset += 1;
        if (callback != NULL){
            win->on_hover = Menu_change_color_hover;
            win->undo_on_hover = Menu_change_color_normal;

            win->data = callback;
            win->data2 = self->self;
            win->data3 = self;
            win->on_mouse_down = Menu_lambda1;
        }
    }
    else
    {
        self->offset += 1;
        win = Window_add_widget(self, self->offset, -1, 0, -1, len, 1, name, 232, 253);
        self->offset += len + 1;
    }
    return win;
}

void Menu_menu_windows_item(struct Window *w, int x, int y)
{
    Tab *tab = w->data;
    Window *menu = w->data2;
    tab_select(tab);
    Window_remove(menu);
}

Window *Menu_menu_windows(Window *self, Tabs *tabs, struct Window *menuItem)
{
    Window *menu = malloc(sizeof *menu);
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    menu->left = menuItem->calculated.x - self->calculated.x;
    menu->top = menuItem->calculated.y - self->calculated.y + 1;
    Tab *tab = tabs->first;
    int i = 0;
    //Window_add_widget(menu, 0, 0, i++, -1, -1, 1, "", 232, 15);
    int maxLen = 0;
    while (tab != NULL)
    {
        char *tab_label = NULL;
        char *selected_sign = " ";
        if (tabs->selected_tab == tab)
            selected_sign = "*";
        asprintf(&tab_label, " %s%s  ", selected_sign, tab->child->id);

        Window *win = Window_add_widget(menu, 0, 0, i++, -1, -1, 1, tab_label, 232, 253);

        win->on_hover = Menu_change_color_hover;
        win->undo_on_hover = Menu_change_color_normal;
        win->data = tab;
        win->data2 = menu;
        win->on_mouse_down = Menu_menu_windows_item;

        maxLen = max(maxLen, strlen(tab_label));
        tab = tab->next;
    }
    Window_add_widget(menu, 0, 0, i++, -1, -1, 1, "", 232, 253);
    menu->width = maxLen;
    menu->height = i;

    Window_append(self, menu);
    //Menu * menu = Menu_create_vertical(self);
}

void Menu_add_windows(Menu *self, char *name, Tabs *tabs)
{
    // Window * Menu_add_element(Menu * self, char * name, Menu_lambda);
    int len = strlen(name);
    self->offset += 1;
    Window *window = Window_add_widget(self, self->offset, -1, 0, -1, len, 1, name, 232, 253);
    self->offset += len + 1;

    window->data = Menu_menu_windows;
    window->data2 = self->parent;
    window->data3 = tabs;
    window->on_mouse_down = Menu_lambda;
}

void Execute_lambda(struct Window *w, int x, int y)
{
    invoke_lambda(w->lambda);
}

void Menu_list_windows_item_selected(Tab *tab, Window *menu)
{
    tab_select(tab);
    Window_remove(menu);
}

Window *Menu_list_windows(Window *menu, Tabs *tabs, struct Window *window_menu_item)
{
    /*Window *menu = malloc(sizeof *menu);
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    menu->left = menuItem->calculated.x - self->calculated.x;
    menu->top = menuItem->calculated.y - self->calculated.y + 1;*/
    Window *submenu = Menu_create_vertical(NULL, menu);

    Tab *tab = tabs->first;
    int i = 0;
    //Window_add_widget(menu, 0, 0, i++, -1, -1, 1, "", 232, 15);
    int maxLen = 0;
    while (tab != NULL)
    {
        char *tab_label = NULL;
        char *selected_sign = " ";
        if (tabs->selected_tab == tab)
            selected_sign = "*";
        asprintf(&tab_label, " %s%s  ", selected_sign, tab->child->id);

        //Window *win = Window_add_widget(menu, 0, 0, i++, -1, -1, 1, tab_label, 232, 253);

        //win->data = tab;
        //win->data2 = menu;
        //win->on_mouse_down = Menu_menu_windows_item;
        Window *win = Menu_add_element(submenu, tab_label, NULL);
        win->on_hover = Menu_change_color_hover;
        win->undo_on_hover = Menu_change_color_normal;
        win->lambda = create_lambda(Menu_list_windows_item_selected, 2, tab, submenu);
        win->on_mouse_down = Execute_lambda;

        maxLen = max(maxLen, strlen(tab_label));
        tab = tab->next;
    }
    Menu_add_element(submenu, "", NULL);
    submenu->left = window_menu_item->left;
    submenu->top = window_menu_item->top + 1;
    submenu->hidden = 0;
    //Window_add_widget(menu, 0, 0, i++, -1, -1, 1, "", 232, 253);
    //menu->width = maxLen;
    //menu->height = i;

    //Window_append(self, menu);
    //Menu * menu = Menu_create_vertical(self);
}

void Menu_add_windows2(Menu *menu, char *name, Tabs *tabs)
{
    // Window * Menu_add_element(Menu * self, char * name, Menu_lambda);
    int len = strlen(name);
    menu->offset += 1;
    Window *window_menu_item = Window_add_widget(menu, menu->offset, -1, 0, -1, len, 1, name, 232, 253);
    menu->offset += len + 1;

    //window->data = Menu_menu_windows;
    //window->data2 = self->parent;
    //window->data3 = tabs;
    window_menu_item->lambda = create_lambda(Menu_list_windows, 3, menu, tabs, window_menu_item);
    window_menu_item->on_mouse_down = Execute_lambda;
}

void Menu_show_submenu(struct Window *w, int x, int y)
{
    Menu *menu = w->data;
    menu->win.hidden = 1 - menu->win.hidden;
}

void Menu_add_submenu(Menu *self, char *name, Menu *menu)
{
    Window *win = Menu_add_element(self, name, NULL);
    win->data = menu;
    win->on_mouse_down = Menu_show_submenu;

    menu->win.left = win->left;
    menu->win.top = win->top + 1;
}
