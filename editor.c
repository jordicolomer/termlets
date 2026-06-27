#include <stdio.h>
#include <stdlib.h>
#include "editor.h"
#include "frame.h"
#include "tabs.h"
#include "slider.h"
#include "logger.h"

// Editor Window

EditorWindow *EditorWindow_new_tab(){
  EditorWindow *w = malloc(sizeof *w);
  Window_init(w, -1, -1, -1, -1, -1, -1);
  w->win.id = "editor tab";

  Window *editor = (Window *) w;

  Window * fm_slider = slider_new(editor);
  fm_slider->left = 0;
  fm_slider->right = 0;
  fm_slider->top = 0;
  fm_slider->bottom = 0;
  Window_append(w, fm_slider);

  return w;
}

void EditorWindow_open_file(EditorWindow * editor_window, char * file_path){
    LOG_INFO("Editor_open_file %s", file_path);
}

// Editor Frame

EditorFrame * last_frame;

Window * Editor_new(int left, int right, int top, int bottom, int width, int height){
    EditorFrame * editor_frame = malloc(sizeof *editor_frame);
    Window *frame = editor_frame;
    //Window *frame = malloc(sizeof *frame);
    Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 0);

    Window *tabs = Tab_new((Window *(*)(void))EditorWindow_new_tab);
    editor_frame->tabs = tabs;
    tabs->top = 0;
    tabs->bottom = 0;
    tabs->left = 0;
    tabs->right = 0;
    Window_append(w, tabs);
    frame->focused = tabs;

    last_frame = frame;

    return frame;
}

void Editor_open_file(EditorFrame * editor_frame, char * file_path){
    EditorWindow * editor_window = tabs_new_tab(editor_frame->tabs);
    EditorWindow_open_file(editor_window, file_path);
}

void Editor_last_open_file(char * file_path){
    if (last_frame == NULL) return;
    Editor_open_file(last_frame, file_path);
}
