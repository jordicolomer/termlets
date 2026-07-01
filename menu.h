#ifndef MENU_H
#define MENU_H
#include "window.h"
#include "tabs.h"

typedef struct Menu
{
  Window win;
  Window * parent;
  int offset;
  int horizontal;
} Menu;

Menu *Menu_create_horizontal(Window * parent);
Menu *Menu_create_vertical(Window * parent);
Window * Menu_add_element(Menu * self, char * name, void * callback);
void Menu_add_windows(Menu * self, char * name, Tabs * tabs);
void Menu_add_submenu(Menu * self, char * name, Menu * menu);

#endif
