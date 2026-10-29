#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "window.h"

typedef struct WM {
    Window win;
  int selected;
  int count;
} WM;


Window *WM_create(int left, int right, int top, int bottom, int width, int height);
Window *WM_show();

extern Window *wm;

#endif
