#ifndef TABS_H
#define TABS_H

#include "window.h"

typedef Window* (*tab_create_callback)(void);

Window *Tab_new(tab_create_callback callback);

#endif