#ifndef FILE_EDITOR_H
#define FILE_EDITOR_H
#include "window.h"

typedef struct EditorWindow {
    struct Window win;
} EditorWindow;


Window *Editor_new(int left, int right, int top, int bottom, int width, int height);
void Editor_open_file(EditorWindow * editor, char * file_path);

#endif
