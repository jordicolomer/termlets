#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include "window.h"

typedef struct ExplorerWindow {
    struct Window win;
    Window *fm;
    Window *slider;
    //int selected;
    Window *selected;
} ExplorerWindow;


Window *FileExplorer_new(int left, int right, int top, int bottom, int width, int height);
void FileExplorer_list_files(ExplorerWindow * self, char * dire);

#endif
