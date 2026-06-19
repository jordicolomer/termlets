#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include "window.h"

Window *FileExplorer_new(int left, int right, int top, int bottom, int width, int height);
Window *FileExplorer_list_files(Window * self, char * dire);

#endif
