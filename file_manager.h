#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include "window.h"
#include <sys/stat.h>

typedef struct FileItemWindow {
    struct Window win;
    char * name;
    char * path;
    off_t size;
    time_t date;
    int is_dir;
    int is_selected;
} FileItemWindow;

typedef struct ExplorerWindow {
    struct Window win;
    Window *fm;
    Window *slider;
    //int selected;
    FileItemWindow *selected; // todo: remove this
    char * path;
    char path_label[1024];
    int sort_by;
    int reversed[3];
    Window * sort_marker[6];
} ExplorerWindow;

typedef struct ExplorerFrame {
    struct Window win;
    Window *tabs;
} ExplorerFrame;


Window *FileExplorer_new(int left, int right, int top, int bottom, int width, int height);
void FileExplorer_list_files(ExplorerWindow * self, char * dire);

#endif
