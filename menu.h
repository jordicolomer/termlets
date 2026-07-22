#ifndef MENU_H
#define MENU_H
#include "window.h"
#include "tabs.h"

typedef struct Menu
{
  Window win;
  //Window * parent;
  //Window * self;
  int offset;
  int vertical;
  int auto_expand;
  struct Menu * submenu;
} Menu;

Menu *Menu_create_horizontal();
Menu *Menu_create_vertical(Window * parent);
Window *Menu_add_element(Menu *self, char *name, Lambda * lambda);
void Menu_add_windows(Menu *menu, char *name, Tabs *tabs, Window * self);
void Menu_add_submenu(Menu * self, char * name, Menu * menu);

#endif
