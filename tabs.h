#ifndef TABS_H
#define TABS_H

#include "window.h"

typedef Window* (*tab_create_callback)(void);

typedef struct Tabs Tabs;
typedef struct Tab Tab;

typedef struct Tab {
    Tabs * parent;
    //Window *terminal;
    Window *tab_label;
    Window *child;
    char str[20];
    Tab * next;

    // this allows navigating all tabs in a single list
    Tab * all_tabs_next;
    Tab * all_tabs_prev;
} Tab;

Tab * all_tabs_head;
Tab * all_tabs_tail;

typedef struct Tabs {
    struct Window win;
    Window *tabs;
    Window *tabs_bar;
    Window *shiftable_tabs;
    int x_offset;
    int idx;
    tab_create_callback callback;
    Tab * selected_tab;
    Tab * first;
    Tab * last;
} Tabs;


Window *Tab_new(tab_create_callback callback, int new_tab);
Window * tabs_new_tab(Tabs *self);
void tab_select(Tab *tab);
void cycle_tab();

#endif