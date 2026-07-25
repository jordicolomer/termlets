#include <stdio.h>
#include <stdlib.h>
#include "text_edit.h"

LineEditorWindow * LineEditorWindow_new(char * c){
    LineEditorWindow *self = malloc(sizeof *self);
    Window_init(self, -1, -1, -1, -1, -1, -1);
    self->win.draw = Widget_draw;

    self->win.top = 0;
    self->win.bottom = 0;
    self->win.left = 0;
    self->win.right = 0;
    self->win.bg = 255;
    self->win.c = &self->buffer;
    self->win.id = "line editor";
    snprintf(self->buffer, sizeof(self->buffer), "edit: %s", c);

    return self;
}