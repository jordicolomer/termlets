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
} Tab;

typedef struct Tabs {
    struct Window win;
    Window *tabs;
    int x_offset;
    int idx;
    tab_create_callback callback;
    Tab * selected_tab;
    Tab * first;
    Tab * last;
} Tabs;


Window *Tab_new(tab_create_callback callback, int new_tab);
Window * tabs_new_tab(Tabs *self);

#endif