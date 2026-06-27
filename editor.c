#include <stdio.h>
#include <stdlib.h>
#include "editor.h"
#include "frame.h"
#include "tabs.h"
#include "slider.h"

EditorWindow *Editor_new_tab(){
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

Window * Editor_new(int left, int right, int top, int bottom, int width, int height){
    Window *frame = malloc(sizeof *frame);
    Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 0);

    Window *tabs = Tab_new((Window *(*)(void))Editor_new_tab);
    tabs->top = 0;
    tabs->bottom = 0;
    tabs->left = 0;
    tabs->right = 0;
    Window_append(w, tabs);
    frame->focused = tabs;

    return frame;
}

void Editor_open_file(EditorWindow * editor, char * file_path){
}
