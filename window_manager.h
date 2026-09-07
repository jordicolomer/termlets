#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "window.h"

Window * WM_create(int left, int right, int top, int bottom, int width, int height);
Window * WM_show();

extern Window * wm;

#endif
