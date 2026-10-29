#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "window.h"
#include "mini_edit.h"
#include "tabs.h"

typedef struct WM {
  Window win;
  int selected;
  Tab *selected_tab;
  int count;
  LineEditorWindow *line_edit;
} WM;


Window *WM_create(int left, int right, int top, int bottom, int width, int height);
Window *WM_show();

extern Window *wm;

#endif
