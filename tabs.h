#ifndef TABS_H
#define TABS_H

#include "window.h"

typedef struct Tabs Tabs;
typedef struct Tab Tab;

typedef Window *(*tab_create_callback)(Tabs *self);

//#define TAB_STR_LEN 1024
//#define TAB_SHORT_STR_LEN 20

typedef struct Tab {
    Tabs *parent;
    // Window *terminal;
    Window *tab_label;
    Window *child;
    char * icon;
    char str[1024];
    char short_str[20];
    Tab *next;
    Tab *prev;

    // this allows navigating all tabs in a single list
    Tab *all_tabs_prev;
    Tab *all_tabs_next;
} Tab;

typedef struct TabWindow {
    struct Window win;
    struct Tab tab;
} TabWindow;

extern Tab *all_tabs_head;
extern Tab *all_tabs_tail;

typedef struct Tabs {
    struct Window win;
    Window *tabs;
    Window *tabs_bar;
    Window *shiftable_tabs;
    int x_offset;
    int idx;
    tab_create_callback callback;
    Tab *selected_tab;
    Tab *first;
    Tab *last;
} Tabs;

Window *Tab_new(tab_create_callback callback, int new_tab);
Window *tabs_new_tab(Tabs *self);
void tab_select(Tab *tab);
void select_tab(Tab *selected_tab, int move);
void cycle_tab();
void cycle_tab_reverse();
void select_window(Window *win);
void tabs_remove_tab(Tab *self);
void Tab_set_title(TabWindow * self, char * path);

#endif
