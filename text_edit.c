#include <stdio.h>
#include <stdlib.h>
#include "text_edit.h"
#include "logger.h"

void LineEditorWindow_send_sequence(struct Window *win, const char *seq, int len){
  LOG_INFO("LineEditorWindow_send_sequence: %s", seq);
}

void LineEditorWindow_send_key(Window * win, char c){
    //LOG_INFO("LineEditorWindow_send_key: %c", c);
    LineEditorWindow *self = win;
    self->buffer[self->cursor] = c;
    self->cursor++;
}

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
    self->win.send_key = LineEditorWindow_send_key;
    self->win.send_sequence = LineEditorWindow_send_sequence;
    self->cursor = 0;
    snprintf(self->buffer, sizeof(self->buffer), "edit: %s", c);

    return self;
}