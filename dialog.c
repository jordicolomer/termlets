#include <stdlib.h>
#include <string.h>
#include "window.h"
#include "dialog.h"

void Dialog_change_color_hover(Window *w, int x, int y)
{
    w->fg = 232;
    w->bg = 27;
}

void Dialog_change_color_normal(Window *w, int x, int y)
{
    w->fg = 232;
    w->bg = 248;
}

void center(Window * parent, Window * child){
    child->left = (parent->calculated.width - child->width)/2;
    child->top = (parent->calculated.height - child->height)/2;
}

void Dialog_close(Window * self){
    Window_remove(self);
}

void Dialog_run_lambda_and_close(Window * self, Lambda * lambda){
    invoke_lambda(lambda);
    Window_remove(self);
}

Window * create_dialog(char * message, Lambda * lambda){
    int fg = 232;
    int bg = 250;

    // dialog
    Window *self = malloc(sizeof *self);
    Window_init(self, -1, -1, -1, -1, -1, -1);
    self->height = 5;
    self->width = 40;
    self->id = "dialog";
    self->bg = bg;
    self->fill = 1;

    // message
    Window * label = Window_add_widget(self, -1, -1, -1, -1, -1, -1, message, fg, bg);
    label->left = 0;
    label->top = 1;
    label->width = strlen(message);
    label->height = 1;
    label->left = (self->width - label->width)/2;

    // cancel
    char * cancel_label = " Cancel ";
    Window * cancel_button = Window_add_widget(self, -1, -1, -1, -1, -1, -1, cancel_label, fg, 248);
    cancel_button->left = 0;
    cancel_button->top = 3;
    cancel_button->width = strlen(cancel_label);
    cancel_button->height = 1;
    cancel_button->left = 5;
    cancel_button->on_hover = Dialog_change_color_hover;
    cancel_button->undo_on_hover = Dialog_change_color_normal;
    cancel_button->lambda = create_lambda(Dialog_close, 1, self);
    cancel_button->on_mouse_down = Window_execute_lambda;

    // ok
    char * ok_label = " Ok ";
    Window * ok_button = Window_add_widget(self, -1, -1, -1, -1, -1, -1, ok_label, fg, 248);
    ok_button->left = 0;
    ok_button->top = 3;
    ok_button->width = strlen(ok_label);
    ok_button->height = 1;
    ok_button->left = -10;
    ok_button->on_hover = Dialog_change_color_hover;
    ok_button->undo_on_hover = Dialog_change_color_normal;
    ok_button->lambda = create_lambda(Dialog_run_lambda_and_close, 2, self, lambda);
    ok_button->on_mouse_down = Window_execute_lambda;

    return self;
}

int open_dialog(Window * parent, char * message, Lambda * lambda){
    Window * dialog = create_dialog(message, lambda);
    center(parent, dialog);
    Window_append(parent, dialog);
}
