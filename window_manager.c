#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#include "window_manager.h"
#include "window.h"
#include "frame.h"
#include "tabs.h"

void WM_init(Window *self){
    int j = 0;

    Tab * tabs = all_tabs_head;
    while (tabs != NULL){
        Window_add_widget(self, 0, 0, j++, -1, -1, -1, tabs->child->id, 232, 255);
        tabs = tabs->all_tabs_next;
    }
}

Window * WM_create(int left, int right, int top, int bottom, int width, int height){
    Window *vm_frame = malloc(sizeof *vm_frame);
    memset(vm_frame, 0, sizeof *vm_frame);  // Zero-initialize to prevent garbage values
    //Window *frame = (Window *)vm_frame;
    Window *w = Frame_init(vm_frame, left, right, top, bottom, width, height, NULL, 0);
    WM_init(w);
    return vm_frame;
}
