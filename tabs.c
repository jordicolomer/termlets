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



void tab_select(Tab *tab){
    change_color_normal(tab->parent->selected_tab->tab_label, 0, 0);
    change_color_hover(tab->tab_label, 0, 0);
    tab->parent->tabs->focused = tab->child;
    Window_bring_to_bottom(tab->child);
    tab->parent->selected_tab = tab;
}

void tab_clicked(Window *wg, int x, int y)
{
    Tab *tab = (Tab *) wg->data;
    tab_select(tab);
}

Window * tabs_new_tab(Tabs *self){
    if (self->selected_tab != NULL) change_color_normal(self->selected_tab->tab_label, 0, 0);

    Window *child = self->callback();
    child->left = 0;
    child->right = 0;
    child->top = 1;
    child->bottom = 0;
    self->tabs->focused = child;

    Window_append(self->tabs, child);

    Tab *mytab = malloc(sizeof *mytab);
    self->selected_tab = mytab;
    if (self->first == NULL) self->first = mytab;
    if (self->last != NULL) self->last->next = mytab;
    self->last = mytab;
    //snprintf(mytab->str, sizeof(mytab->str), " %d ", self->idx+1);
    //snprintf(mytab->str, sizeof(mytab->str), " %s ", child->id);
    //mytab->str = child->id;
    self->idx++;
    char * label = child->id;
    Window * tab = Window_add_widget(self->tabs, -1, -1, -1, -1, -1, -1, label, 232, 255);
    tab->left = self->x_offset;
    tab->top = 0;
    tab->height = 1;
    //LOG_INFO("tabs_new_tab %s %d", label, strlen(label));
    //tab->width = strlen(label)+1;
    tab->width = ID_LENGTH-3; // there is a 4 byte 2 wide char + null so -3. todo fix this
    tab->on_mouse_down = tab_clicked;
    tab->on_hover = change_color_hover;
    tab->undo_on_hover = change_color_normal;

    mytab->parent = self;
    //mytab->terminal = terminal;
    mytab->child = child;
    mytab->tab_label = tab;
    tab->data = mytab;

    self->x_offset += tab->width + 1;

    change_color_hover(tab, 0, 0);

    return child;

    //tab_select(mytab);
}

void tabs_cycle(Tabs *self){
    Tab * current = self->selected_tab;
    if (current->next != NULL) current = current->next;
    else current = self->first;
    tab_select(current);
}

void tabs_plus_clicked(Window *wg, int x, int y)
{
    Tabs *mytab = (Tabs *) wg->data;
    tabs_new_tab(mytab);
}

void tabs_send_key(struct Window *wg, char c)
{
    if (c == 14){ // Ctrl+N
        Tabs *mytab = (Tabs *) wg->data;
        tabs_new_tab(mytab);
        return;
    }
    if (c == 12){ // Ctrl+L
        Tabs *mytab = (Tabs *) wg->data;
        tabs_cycle(mytab);
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

Window *Tab_new(tab_create_callback callback, int new_tab){
    Tabs *mytab = malloc(sizeof *mytab);
    //Window *tabs = malloc(sizeof *tabs);
    Window *tabs = mytab;
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

    if (new_tab) tabs_new_tab(mytab);
    return tabs;
}