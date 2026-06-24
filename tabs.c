#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tabs.h"
#include "logger.h"

void change_color_hover(Window *wg, int x, int y)
{
    //LOG_INFO("change_color_hover");
    wg->fg = 236;
    wg->bg = 250;
}

void change_color_normal(Window *wg, int x, int y)
{
    //LOG_INFO("change_color_normal");
    wg->fg = 232;
    wg->bg = 254;
}

typedef struct Tabs {
    Window *tabs;
    int x_offset;
    int idx;
    tab_create_callback callback;
} Tabs;

typedef struct Tab {
    Tabs * parent;
    //Window *terminal;
    Window *child;
    char str[20];
} Tab;

void tab_clicked(Window *wg, int x, int y)
{
    Tab *tab = (Tab *) wg->data;
    tab->parent->tabs->focused = tab->child;
    Window_bring_to_bottom(tab->child);
}

void tabs_new_tab(Tabs *self){
    Window *child = self->callback();
    child->left = 0;
    child->right = 0;
    child->top = 1;
    child->bottom = 0;
    self->tabs->focused = child;

    Window_append(self->tabs, child);

    Tab *mytab = malloc(sizeof *mytab);
    snprintf(mytab->str, sizeof(mytab->str), " %d ", self->idx+1);
    self->idx++;
    Window * tab = Window_add_widget(self->tabs, -1, -1, -1, -1, -1, -1, mytab->str, 232, 255);
    tab->left = self->x_offset;
    tab->top = 0;
    tab->height = 1;
    tab->width = strlen(mytab->str)+1;
    tab->on_mouse_down = tab_clicked;
    tab->on_hover = change_color_hover;
    tab->undo_on_hover = change_color_normal;

    mytab->parent = self;
    //mytab->terminal = terminal;
    mytab->child = child;
    tab->data = mytab;

    self->x_offset += tab->width;

}

void tabs_plus_clicked(Window *wg, int x, int y)
{
    Tabs *mytab = (Tabs *) wg->data;
    tabs_new_tab(mytab);
}

void tabs_send_key(struct Window *wg, char c)
{
    if (c == 12){ // Ctrl+L
        Tabs *mytab = (Tabs *) wg->data;
        //LOG_INFO("tabs_send_key %p", mytab);
        tabs_new_tab(mytab);
        return;
    }

    Window *focused_cursor = wg->focused;
    if (focused_cursor != NULL) while (focused_cursor->send_key == NULL && focused_cursor->focused != NULL) focused_cursor = focused_cursor->focused;

    if (focused_cursor != NULL && focused_cursor->send_key != NULL) {
        focused_cursor->send_key(focused_cursor, c);
    }

}

/*
void tabs_send_sequence(struct Window *wg, const char *seq, int len)
{

}
*/

Window *Tab_new(tab_create_callback callback){
    Window *tabs = malloc(sizeof *tabs);
    Window_init(tabs, -1, -1, -1, -1, -1, -1);
    tabs->send_key = tabs_send_key;
    //tabs->send_sequence = tabs_send_sequence;

    tabs->id = "tabs";

    Window *tabs_bar = malloc(sizeof *tabs_bar);
    Window_init(tabs_bar, -1, -1, -1, -1, -1, -1);
    tabs_bar->left = 0;
    tabs_bar->right = 0;
    tabs_bar->top = 0;
    tabs_bar->height = 1;
    Window_append(tabs, tabs_bar);

    Tabs *mytab = malloc(sizeof *mytab);
    mytab->tabs = tabs;
    tabs->data = mytab;
    mytab->callback = callback;

    Window * bg = Window_add_widget(tabs_bar, -1, -1, -1, -1, -1, -1, "   ", 232, 255);
    bg->left = 0;
    bg->top = 0;
    bg->left = 0;
    bg->right = 0;
    bg->height = 1;

    Window * plus_button = Window_add_widget(tabs_bar, -1, -1, -1, -1, -1, -1, " + ", 232, 255);
    plus_button->left = mytab->x_offset;
    plus_button->top = 0;
    plus_button->width = 3;
    plus_button->height = 1;
    plus_button->on_hover = change_color_hover;
    plus_button->undo_on_hover = change_color_normal;
    plus_button->on_mouse_down = tabs_plus_clicked;
    plus_button->data = mytab;
    mytab->x_offset += plus_button->width;

    tabs_new_tab(mytab);
    return tabs;
}