#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "utils.h"

Menu *Menu_create_horizontal(Window * parent){

}

Menu *Menu_create_vertical(Window * parent){
    Menu *menu = malloc(sizeof *menu);
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    menu->win.left = 0;
    menu->win.right = 0;
    menu->win.top = 0;
    menu->win.height = 1;
    menu->parent = parent;
    
    // background
    Window_add_widget(menu, 0, 0, 0, -1, -1, 1, "", 232, 253);

    return menu;
}

Window * Menu_add_element(Menu * self, char * name, void * callback){
    int len = strlen(name);
    self->x_offset += 1;
    Window_add_widget(self, self->x_offset, -1, 0, -1, len, 1, name, 232, 253);
    self->x_offset += len + 1;
}


void Editor_change_color_hover(Window *wg, int x, int y)
{
    wg->fg = 232;
    wg->bg = 27;
}

void Editor_change_color_normal(Window *wg, int x, int y)
{
    wg->fg = 232;
    wg->bg = 15;
}

void Editor_menu_windows_item(struct Window *w, int x, int y){
    Tab * tab = w->data;
    Window *menu = w->data2;
    tab_select(tab);
    Window_remove(menu);
}

Window *Editor_menu_windows(Window * self, Tabs * tabs, struct Window *menuItem){
    Window *menu = malloc(sizeof *menu);
    Window_init(menu, -1, -1, -1, -1, -1, -1);
    menu->left = menuItem->calculated.x - self->calculated.x;
    menu->top = menuItem->calculated.y - self->calculated.y + 1;
    //Tabs * tabs = self->tabs->data;
    Tab * tab = tabs->first;
    int i = 0;
    Window_add_widget(menu, 0, 0, i++, -1, -1, 1, "", 232, 15);
    int maxLen = 0;
    while (tab != NULL){
        char * tab_label = NULL;
        char * selected_sign = " ";
        if (tabs->selected_tab == tab) selected_sign = "*";
        asprintf(&tab_label, " %s%s  ", selected_sign, tab->child->id);
        
        Window * win = Window_add_widget(menu, 0, 0, i++, -1, -1, 1, tab_label, 232, 15);

        win->on_hover = Editor_change_color_hover;
        win->undo_on_hover = Editor_change_color_normal;
        win->data = tab;
        win->data2 = menu;
        win->on_mouse_down = Editor_menu_windows_item;

        maxLen = max(maxLen, strlen(tab_label));
        tab = tab->next;
    }
    Window_add_widget(menu, 0, 0, i++, -1, -1, 1, "", 232, 15);
    menu->width = maxLen;
    menu->height = i;

    Window_append(self, menu);
}

void Editor_lambda(struct Window *w, int x, int y){
  Window *(*fn)(Window *, Tabs *, struct Window *) = (Window *(*)(Window *, Tabs *, struct Window *))w->data;
  Window * self = w->data2;
  Window * tabs = w->data3;
  fn(self, tabs, w);
}

void Menu_add_windows(Menu * self, char * name, Tabs * tabs){
    //Window * Menu_add_element(Menu * self, char * name, Editor_lambda);
    int len = strlen(name);
    self->x_offset += 1;
    Window * window = Window_add_widget(self, self->x_offset, -1, 0, -1, len, 1, name, 232, 253);
    self->x_offset += len + 1;

    window->data = Editor_menu_windows;
    window->data2 = self->parent;
    window->data3 = tabs;
    window->on_mouse_down = Editor_lambda;
}

void Menu_add_submenu(Menu * self, char * name, Menu * menu){

}
