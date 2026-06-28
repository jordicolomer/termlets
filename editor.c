#include <stdio.h>
#include <stdlib.h>
#include "editor.h"
#include "frame.h"
#include "tabs.h"
#include "slider.h"
#include "logger.h"
#include "string.h"
#include "buffer.h"
#include "utils.h"
#include "taskbar.h"

// Editor Window
 
void EditorWindow_make_cursor_visible(EditorWindow * self){
    int height = self->win.calculated.height;
    int diff = self->cursor_n + self->win.shift;
    if (diff < 0) self->win.shift = -(self->cursor_n);
    if (diff > height - 1) self->win.shift = height - 1 - self->cursor_n;
}

void EditorWindow_send_key(Window * win, char c)
{
    EditorWindow * self = win;
    if (c == 106){ // j
        self->cursor_n++;
        self->cursor_n = min(self->cursor_n, self->n_lines-1);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 107){ // k
        self->cursor_n--;
        self->cursor_n = max(self->cursor_n, 0);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 117){ // u
        self->cursor_n += win->calculated.height;
        self->cursor_n = min(self->cursor_n, self->n_lines-1);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 105){ // i
        self->cursor_n -= win->calculated.height;
        self->cursor_n = max(self->cursor_n, 0);
        EditorWindow_make_cursor_visible(self);
        return;
    }
}



void EditorWindow_draw(struct Window *w, int hasFocus){
    //LOG_INFO("EditorWindow_draw");
    //int first_visible_line = -w->shift;
    EditorWindow *self = w;
    //self->cursor_n = first_visible_line;
    //EditorWindow_make_cursor_visible(self);
    //EditorWindow_update_first_visible_line(self, first_visible_line);

    Geometry geo = w->calculated;
    int i = 0;
    //Node * current = self->top;
    Node * current = self->head;
    for(int i=0;i<-self->win.shift;i++) current = current->next;

    while(i < geo.height){
        int bg = 239;
        if (-self->win.shift + i == self->cursor_n) bg = 53;

        char * str = "";
        if (current != NULL) str = current->line;
        Buffer_print_raw(&main_buf, geo.y + i++, geo.x, geo.width, str, 15, bg);
        if (current != NULL) current = current->next;
    }
}

EditorWindow * latestEditorWindow;
EditorWindow *EditorWindow_new(){
    EditorWindow *self = malloc(sizeof *self);
    Window_init(self, -1, -1, -1, -1, -1, -1);
    self->win.top = 0;
    self->win.bottom = 0;
    self->win.left = 0;
    self->win.right = 0;
    self->win.id = "editor tab";

    //Window *editor = (Window *) self;
    self->win.draw = EditorWindow_draw;

    self->win.send_key = EditorWindow_send_key;

    latestEditorWindow = self;

    return self;
}

Window *EditorWindow_new_tab(){
    EditorWindow * editor = EditorWindow_new();
    Window *slider = slider_new(editor);
    Slider_show_grip(slider);
    return slider;
}
/*
Window *EditorWindow_new_tab(){
  EditorWindow *w = malloc(sizeof *w);
  Window_init(w, -1, -1, -1, -1, -1, -1);
  w->win.id = "editor tab";

  Window *editor = (Window *) w;
  editor->draw = EditorWindow_draw;

  Window * fm_slider = slider_new(editor);
  fm_slider->left = 0;
  fm_slider->right = 0;
  fm_slider->top = 0;
  fm_slider->bottom = 0;
  //Window_append(w, fm_slider);

  w->win.send_key = EditorWindow_send_key;

  return fm_slider;
}
*/

#include <ctype.h>
void replace_nonprintable(char *str)
{
    while (*str) {
        if (!isprint((unsigned char)*str)) {
            *str = '?';
        }
        str++;
    }
}

Node* create_node(const char *text) {
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) {
        perror("malloc failed");
        exit(1);
    }
    replace_nonprintable(text);
    new_node->line = strdup(text);  // copy string
    new_node->next = NULL;
    new_node->prev = NULL;

    return new_node;
}

void append(EditorWindow * self, const char *text) {
    Node *new_node = create_node(text);

    if (self->head == NULL) {
        self->head = new_node;
        //return;
    }
    if (self->tail != NULL) self->tail->next = new_node;
    new_node->prev = self->tail;
    self->tail = new_node;

}

#define MAX_LINE 10240

void load_file(EditorWindow * self, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen failed");
        return;
    }

    //Node *head = NULL;
    //Node *tail = NULL;
    char buffer[MAX_LINE];

    while (fgets(buffer, MAX_LINE, file)) {
        // Optional: remove newline
        buffer[strcspn(buffer, "\n")] = '\0';

        append(self, buffer);
        self->n_lines++;
    }

    fclose(file);

    self->win.virtual_height = self->n_lines;
    //return head;
}

void EditorWindow_open_file(EditorWindow * editor_window, char * file_path){
    load_file(editor_window, file_path);
    editor_window->top = editor_window->head;
    LOG_INFO("Editor_open_file %s", file_path);
}

// Editor Frame

EditorFrame * last_frame;

Window *Editor_menu(){
  Window *menu = malloc(sizeof *menu);
  Window_init(menu, -1, -1, -1, -1, -1, -1);
  menu->left = 0;
  menu->right = 0;
  menu->top = 0;
  menu->height = 1;

  int j = 0;
  int x_offset = 0;
  int widget_width = 6;
  Window_add_widget(menu, x_offset, -1, j, -1, widget_width, 1, " File", 232, 253);
  x_offset += widget_width;
  widget_width = 6;
  Window_add_widget(menu, x_offset, -1, j, -1, widget_width, 1, " Edit", 232, 253);
  x_offset += widget_width;
  widget_width = 6;
  Window_add_widget(menu, x_offset, -1, j, -1, widget_width, 1, " View", 232, 253);
  x_offset += widget_width;
  widget_width = 6;
  Window_add_widget(menu, x_offset, 0, j, -1, -1, 1, " Help", 232, 253);
  x_offset += widget_width;

  return menu;
}

Window *Editor_toolbar(){
  Window *toolbar = malloc(sizeof *toolbar);
  Window_init(toolbar, -1, -1, -1, -1, -1, -1);
  toolbar->left = 0;
  toolbar->right = 0;
  toolbar->top = 1;
  toolbar->height = 1;

  int x_offset = 0;
  int widget_width = 12;
  int j = 0;
  Window_add_widget(toolbar, x_offset, -1, j, -1, widget_width, 1, " 📄 New File", 232, 254);
  x_offset += widget_width;
  widget_width = 12;
  Window_add_widget(toolbar, x_offset, -1, j, -1, widget_width, 1, "📁 New Dir", 232, 254);
  x_offset += widget_width;
  widget_width = 8;
  Window_add_widget(toolbar, x_offset, -1, j, -1, widget_width, 1, "📋 Copy", 232, 254);
  x_offset += widget_width;
  widget_width = 8;
  Window_add_widget(toolbar, x_offset, -1, j, -1, widget_width, 1, "🔪 Cut", 232, 254);
  x_offset += widget_width;
  widget_width = 10;
  Window_add_widget(toolbar, x_offset, -1, j, -1, widget_width, 1, "📌 Paste", 232, 254);
  x_offset += widget_width;
  widget_width = 10;
  Window_add_widget(toolbar, x_offset, -1, j, -1, widget_width, 1, "🔤 Rename", 232, 254);
  x_offset += widget_width;
  widget_width = 10;
  Window_add_widget(toolbar, x_offset, 0, j, -1, -1, 1, "❌ Delete", 232, 254);
  x_offset += widget_width;
  

  return toolbar;
}

Window * Editor_new(int left, int right, int top, int bottom, int width, int height){
    EditorFrame * editor_frame = malloc(sizeof *editor_frame);
    Window *frame = editor_frame;
    //Window *frame = malloc(sizeof *frame);
    Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 0);

    Window * menu = Editor_menu();
    Window_append(w, menu);
    Window * toolbar = Editor_toolbar();
    Window_append(w, toolbar);

    Window *tabs = Tab_new((Window *(*)(void))EditorWindow_new_tab, 0);
    editor_frame->tabs = tabs;
    tabs->top = 2;
    tabs->bottom = 0;
    tabs->left = 0;
    tabs->right = 0;
    Window_append(w, tabs);
    frame->focused = tabs;

    last_frame = frame;

    return frame;
}

void Editor_open_file(EditorFrame * editor_frame, char * file_path){
    Window * slider = tabs_new_tab(editor_frame->tabs);
    //EditorWindow * editor_window = slider->focused;
    //EditorWindow * editor_window = tabs_new_tab(editor_frame->tabs);
    EditorWindow_open_file(latestEditorWindow, file_path);
}

void Editor_last_open_file(char * file_path){
    if (last_frame == NULL){
        file_editor_new();
        //return;
    }
    //LOG_INFO("Editor_last_open_file %p", last_frame);
    Editor_open_file(last_frame, file_path);
    //Window_bring_to_bottom(last_frame);
    //root->focused = last_frame;
    TaskBar_switch_frame(last_frame);
}
