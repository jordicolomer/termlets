#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tabs.h"
#include "logger.h"
#include "buffer.h"
#include "common.h"

//#define SELECTED_COLOR 33

void change_color_hover(Window *wg, int x, int y)
{

    //LOG_INFO("change_color_hover");
    wg->fg = 236;
    wg->bg = 250;

    Tab *tab = wg->data;
    if (tab->parent->selected_tab == tab){
        wg->bg = SELECTED_COLOR;
    }
}

void change_color_normal(Window *wg, int x, int y)
{
    //LOG_INFO("change_color_normal");
    wg->fg = 232;
    wg->bg = 254;

    Tab *tab = wg->data;
    if (tab->parent->selected_tab == tab){
        wg->bg = SELECTED_COLOR;
    }
}

void make_visible(Tab *tab){
    Window * label = tab->tab_label;
    //Window * shiftable_tabs = label->parent;
    Window * shiftable_tabs = tab->parent->shiftable_tabs;
    if (shiftable_tabs->calculated.width == 0) return; // if we haven't drawn yet
    //LOG_INFO("make_visible %s label->left:%d shiftable_tabs->calculated.width:%d label->width:%d label->calculated.width:%d shiftable_tabs->shift_x:%d", shiftable_tabs->id, label->left, shiftable_tabs->calculated.width, label->width, label->calculated.width, shiftable_tabs->shift_x);
    if (label->left + shiftable_tabs->shift_x < 0){
        shiftable_tabs->shift_x = -label->left;
        //LOG_INFO("if1 shiftable_tabs->shift_x %d", shiftable_tabs->shift_x);
        return;
    }
    if (shiftable_tabs->calculated.width < label->left + label->width + shiftable_tabs->shift_x){
        shiftable_tabs->shift_x = - (label->left - shiftable_tabs->calculated.width + label->width);
        //LOG_INFO("if2 shiftable_tabs->shift_x %d", shiftable_tabs->shift_x);
    }
}

void tab_select(Tab *tab){
    Window * prev = tab->parent->selected_tab->tab_label;
    //change_color_normal(tab->parent->selected_tab->tab_label, 0, 0);
    //change_color_hover(tab->tab_label, 0, 0);
    tab->parent->tabs->focused = tab->child;
    Window_bring_to_bottom(tab->child);
    tab->parent->selected_tab = tab;
    make_visible(tab);

    change_color_normal(prev, 0, 0);
    change_color_normal(tab->tab_label, 0, 0);
}

void tab_clicked(Window *wg, int x, int y)
{
    Tab *tab = (Tab *) wg->data;
    tab_select(tab);
}

Window * tabs_new_tab(Tabs *self){
    Window * prev_tab_label = NULL;
    if (self->selected_tab != NULL) prev_tab_label = self->selected_tab->tab_label;
    //change_color_normal(self->selected_tab->tab_label, 0, 0);

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
    Window * tab = Window_add_widget(self->shiftable_tabs, -1, -1, -1, -1, -1, -1, label, 232, 255);
    tab->left = self->x_offset;
    tab->top = 0;
    tab->height = 1;
    //LOG_INFO("tabs_new_tab %s %d", label, strlen(label));
    //tab->width = strlen(label)+1;
    //tab->width = ID_LENGTH-3; // there is a 4 byte 2 wide char + null so -3. todo fix this
    tab->width = ID_LENGTH;
    //tab->width = calculate_width(label)+1;
    tab->on_mouse_down = tab_clicked;
    tab->on_hover = change_color_hover;
    tab->undo_on_hover = change_color_normal;
    tab->data = mytab;

    mytab->parent = self;
    //mytab->terminal = terminal;
    mytab->child = child;
    mytab->tab_label = tab;
    tab->data = mytab;

    self->x_offset += tab->width;

    change_color_hover(tab, 0, 0);
    if (prev_tab_label != NULL) change_color_normal(prev_tab_label, 0, 0);

    make_visible(mytab);

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

void tabs_scroll_wheel_up(struct Window *wg)
{
    Window *focused_cursor = wg->focused;
    if (focused_cursor != NULL) while (focused_cursor->scroll_wheel_up == NULL && focused_cursor->focused != NULL) focused_cursor = focused_cursor->focused;

    if (focused_cursor != NULL && focused_cursor->scroll_wheel_up != NULL) {
        focused_cursor->scroll_wheel_up(focused_cursor);
    }
}

void tabs_scroll_wheel_down(struct Window *wg)
{
    Window *focused_cursor = wg->focused;
    if (focused_cursor != NULL) while (focused_cursor->scroll_wheel_down == NULL && focused_cursor->focused != NULL) focused_cursor = focused_cursor->focused;

    if (focused_cursor != NULL && focused_cursor->scroll_wheel_down != NULL) {
        focused_cursor->scroll_wheel_down(focused_cursor);
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
    tabs->scroll_wheel_up = tabs_scroll_wheel_up;
    tabs->scroll_wheel_down = tabs_scroll_wheel_down;

    tabs->id = "tabs";

    Window *tabs_bar = malloc(sizeof *tabs_bar);
    tabs_bar->id = "tabs_bar";
    mytab->tabs_bar = tabs_bar;
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
    //mytab->x_offset += plus_button->width;
    mytab->x_offset = 0;

    Window *shiftable_tabs = malloc(sizeof *shiftable_tabs);
    shiftable_tabs->fill = 1;
    shiftable_tabs->bg = 253;
    shiftable_tabs->id = "shiftable_tabs";
    mytab->shiftable_tabs = shiftable_tabs;
    Window_init(shiftable_tabs, -1, -1, -1, -1, -1, -1);
    shiftable_tabs->left = plus_button->width;
    shiftable_tabs->right = 0;
    shiftable_tabs->top = 0;
    shiftable_tabs->height = 1;
    Window_append(tabs_bar, shiftable_tabs);

    if (new_tab) tabs_new_tab(mytab);
    return tabs;
}