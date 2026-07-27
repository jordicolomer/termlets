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
#include "lambda.h"
#include "menu.h"
#include "text_edit.h"
#include "clipboard.h"

// Editor Window

void EditorWindow_scroll_wheel_down(struct Window *w){
  EditorWindow * self = w;
  Slider_scroll_down(self->slider);
}

void EditorWindow_scroll_wheel_up(struct Window *w){
  EditorWindow * self = w;
  Slider_scroll_up(self->slider);
}

void EditorWindow_make_cursor_visible(EditorWindow *self)
{
    int height = self->win.calculated.height;
    int diff = self->cursor_n + self->win.shift;
    if (diff < 0)
        self->win.shift = -(self->cursor_n);
    if (diff > height - 1)
        self->win.shift = height - 1 - self->cursor_n;
}

Node * EditorWindow_get_line_number(EditorWindow *self, int number){
    // todo: make this efficient
    Node *current = self->head;
    for (int i = 0; i < number; i++){
        if (current == NULL) return current;
        current = current->next;
    }
    return current;
}

void double_capacity(Node *node)
{
    size_t new_capacity = node->capacity * 2;

    char *new_line = realloc(node->line, new_capacity);
    if (new_line == NULL) {
        return; // or handle allocation failure
    }

    node->line = new_line;
    node->capacity = new_capacity;
}

void EditorWindow_insert(EditorWindow *self, char c){
    Node * node = EditorWindow_get_line_number(self, self->cursor_n);
    if (node->capacity <= node->length+1){
        double_capacity(node);
    }
    insert_char(node->line, self->cursor_x, c, node->length, node->capacity);
    node->length++;
    self->cursor_x++;
}

void Node_append(Node *node, char *text)
{
    if (!node || !text || *text == '\0')
        return;

    size_t text_len = strlen(text);

    // Check if we need to grow the buffer
    if (node->length + text_len + 1 > node->capacity)
    {
        // Double the capacity or allocate enough for the new text
        size_t new_capacity = node->capacity * 2;
        if (new_capacity < node->length + text_len + 1)
            new_capacity = node->length + text_len + 1 + 16; // small extra padding

        char *new_line = realloc(node->line, new_capacity);
        if (!new_line)
            return; // allocation failed

        node->line = new_line;
        node->capacity = new_capacity;
    }

    // Append the text
    memcpy(node->line + node->length, text, text_len);
    node->length += text_len;
    node->line[node->length] = '\0';
}

void EditorWindow_delete(EditorWindow *self){
    if (self->cursor_x == 0){
        if (self->cursor_n != 0){
            Node * node = EditorWindow_get_line_number(self, self->cursor_n);
            Node * next = node->next;
            Node * prev = node->prev;
            self->cursor_x = prev->length;
            self->cursor_n--;
            self->n_lines--;
            Node_append(prev, node->line);
            prev->next = next;
            if (next == NULL) self->tail = next;
            else next->prev = prev;
        }
        return;
    } else {
        Node * node = EditorWindow_get_line_number(self, self->cursor_n);
        delete_char(node->line, self->cursor_x-1, node->length);
        node->length--;
        self->cursor_x--;
    }
}

void EditorWindow_copy(EditorWindow *self){
    if (self->selection_n == -1) return;
    // get range
    int i1 = self->cursor_n;
    int j1 = self->cursor_x;
    int i2 = self->selection_n;
    int j2 = self->selection_x;
    if (self->selection_n < self->cursor_n || 
       (self->selection_n == self->cursor_n && self->selection_x < self->cursor_x)){
        i1 = self->selection_n;
        j1 = self->selection_x;
        i2 = self->cursor_n;
        j2 = self->cursor_x;
    }

    // calculate size
    int size = 1;
    Node * first_line = EditorWindow_get_line_number(self, i1);
    Node * node = first_line;
    for(int i=0;i<i2-i1+1;i++){
        size += node->length + 1;
        node = node->next;
    }

    // create buffer and copy data
    char * buffer = malloc(size);
    char * buffer_cursor = buffer;
    node = first_line;
    for(int i=0;i<i2-i1+1;i++){
        if (i == 0){ // first item
            if (i == i2-i1){ // first and last item
                memcpy(buffer_cursor, node->line+j1, j2-j1);
                buffer_cursor += j2-j1;
            } else { // first item only
                memcpy(buffer_cursor, node->line+j1, node->length-j1);
                buffer_cursor += node->length-j1;
                *buffer_cursor = '\n';
                buffer_cursor++;
            }
        } else if (i == i2-i1){ // last item
            memcpy(buffer_cursor, node->line, j2);
            buffer_cursor += j2;
        } else { // rest
            memcpy(buffer_cursor, node->line, node->length);
            buffer_cursor += node->length;
            *buffer_cursor = '\n';
            buffer_cursor++;
        }
        node = node->next;
    }
    *buffer_cursor = '\0';

    clipboard_copy(buffer);
    free(buffer);
}

void EditorWindow_paste(EditorWindow *self){
    Node * current = EditorWindow_get_line_number(self, self->cursor_n);
    Node * next_line = current->next;
    char * cb = clipboard_paste_apple();

    char * tail = strdup(current->line + self->cursor_x);
    current->line[self->cursor_x] = '\0';
    current->length = self->cursor_x;

    char *line = cb;
    int i = 0;
    while (line != NULL/* && *line != '\0'*/) {
        char *end = strchr(line, '\n');

        if (end != NULL) {
            *end = '\0';  // temporarily terminate this line
        }

        if (i == 0){ // first line
            Node_append(current, line);
        } else {
            Node *new_node = malloc(sizeof(Node));
            new_node->line = strdup(line);
            new_node->length = strlen(line);
            new_node->capacity = new_node->length+1;
            current->next = new_node;
            new_node->prev = current;
            current = new_node;
        }

        if (end == NULL || *line == '\0'){
            break;
        }

        line = end + 1;
        i += 1;
    }
    self->cursor_x = current->length;
    Node_append(current, tail);
    free(tail);
    
    current->next = next_line;
    next_line->prev = current;

    self->cursor_n += i;

    free(cb);
}

void EditorWindow_delete_region(EditorWindow *self){
    if (self->selection_n == -1) return;
    // get range
    int i1 = self->cursor_n;
    int j1 = self->cursor_x;
    int i2 = self->selection_n;
    int j2 = self->selection_x;
    if (self->selection_n < self->cursor_n || 
       (self->selection_n == self->cursor_n && self->selection_x < self->cursor_x)){
        i1 = self->selection_n;
        j1 = self->selection_x;
        i2 = self->cursor_n;
        j2 = self->cursor_x;
    }
    
    Node * node1 = EditorWindow_get_line_number(self, i1);
    Node * node2 = EditorWindow_get_line_number(self, i2);
    char * tail = node2->line + j2;
    node1->line[j1] = 0;
    node1->length = j1;
    Node_append(node1, tail);

    node1->next = node2->next;
    if (node2->next == NULL){
        self->tail = node1;
    } else {
        node2->next->prev = node1;
    }

    self->cursor_n = i1;
    self->cursor_x = j1;

    self->selection_n = -1;
}

void EditorWindow_cut(EditorWindow *self){
    EditorWindow_copy(self);
    EditorWindow_delete_region(self);
}

void EditorWindow_save(EditorWindow *self){
    FILE *file = fopen(self->file_path, "w");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    Node *current = self->head;
    while(current != NULL){
        //LOG_INFO("EditorWindow_save %s", current->line);
        fputs(current->line, file);
        current = current->next;
        if (current != NULL) fputc('\n', file);
    }
    fclose(file);
}

void EditorWindow_newline(EditorWindow *self){
    Node * node = EditorWindow_get_line_number(self, self->cursor_n);
    Node * next = node->next;

    Node *new_node = malloc(sizeof(Node));
    if (!new_node)
    {
        perror("malloc failed");
        return;
    }
    node->next = new_node;

    new_node->line = strdup(node->line + self->cursor_x);
    new_node->length = strlen(new_node->line);
    new_node->capacity = new_node->length + 1;
    new_node->next = next;
    new_node->prev = node;

    if (next == NULL) self->tail = new_node;
    else next->prev = new_node;

    node->line[self->cursor_x] = '\0';
    node->length = self->cursor_x;

    self->n_lines++;
    self->cursor_n++;
    self->cursor_x = 0;
}

void EditorWindow_fix_cursor_x(EditorWindow *self){
    Node * node = EditorWindow_get_line_number(self, self->cursor_n);
    if (node->length < self->cursor_x){
        self->cursor_x = node->length;
    }
}

void EditorWindow_send_key(Window *win, char c)
{
    EditorWindow *self = win;
    if (c == ';')
    {
        self->edit_mode = 1 - self->edit_mode;
        return;
    }
    if (c == 8) // Backspace
    {
        EditorWindow_delete(self);
        return;
    }
    if (c == 13) // Carriage Return (Enter)
    {
        EditorWindow_newline(self);
        return;
    }
    if (self->edit_mode == 1){
        EditorWindow_insert(self, c);
        return;
    }
    if (c == 's')
    {
        self->cursor_x = 0;
        return;
    }
    if (c == 'd')
    {
        self->cursor_x = max(self->cursor_x - 1, 0);
        return;
    }
    if (c == 'f')
    {
        Node * node = EditorWindow_get_line_number(self, self->cursor_n);
        int mx = strlen(node->line);
        self->cursor_x = min(self->cursor_x + 1, mx);
        return;
    }
    if (c == 'g')
    {
        Node * node = EditorWindow_get_line_number(self, self->cursor_n);
        int mx = strlen(node->line);
        self->cursor_x = mx;
        return;
    }
    if (c == 'j')
    {
        self->cursor_n++;
        self->cursor_n = min(self->cursor_n, self->n_lines - 1);
        EditorWindow_fix_cursor_x(self);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 'h')
    {
        self->cursor_n = self->n_lines - 1;
        EditorWindow_fix_cursor_x(self);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 'k')
    {
        self->cursor_n--;
        self->cursor_n = max(self->cursor_n, 0);
        EditorWindow_fix_cursor_x(self);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 'l')
    {
        self->cursor_n = 0;
        EditorWindow_fix_cursor_x(self);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 'u')
    {
        self->cursor_n += win->calculated.height;
        self->cursor_n = min(self->cursor_n, self->n_lines - 1);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 'i')
    {
        self->cursor_n -= win->calculated.height;
        self->cursor_n = max(self->cursor_n, 0);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 'p')
    {
        self->selection_n = self->cursor_n;
        self->selection_x = self->cursor_x;
        return;
    }
    if (c == 'c')
    {
        EditorWindow_copy(self);
        self->selection_n = -1;
        return;
    }
    if (c == 'v')
    {
        EditorWindow_paste(self);
        return;
    }
    if (c == 'x')
    {
        EditorWindow_cut(self);
        return;
    }
    if (c == 'w')
    {
        EditorWindow_save(self);
        return;
    }
}


void EditorWindow_draw(struct Window *w, int hasFocus)
{
    // LOG_INFO("EditorWindow_draw");
    // int first_visible_line = -w->shift;
    EditorWindow *self = w;
    // self->cursor_n = first_visible_line;
    // EditorWindow_make_cursor_visible(self);
    // EditorWindow_update_first_visible_line(self, first_visible_line);

    Geometry geo = w->calculated;
    int i = 0;
    // Node * current = self->top;
    /*Node *current = self->head;
    for (int i = 0; i < -self->win.shift; i++)
        current = current->next;*/
    int top_line = -self->win.shift;
    Node *current = EditorWindow_get_line_number(self, top_line);

    while (i < geo.height)
    {
        int bg = 255;
        if (bg >= 232 + 4 && !hasFocus)
            bg -= 4;

        char *str = "";
        if (current != NULL)
            str = current->line;
        
        // if in between selection, show blue bg
        if (self->selection_n != -1){
            if (self->cursor_n < top_line+i && top_line+i < self->selection_n) bg = 27;
            if (self->selection_n < top_line+i && top_line+i < self->cursor_n) bg = 27;
        }

        Buffer_print(&main_buf, geo.y + i, geo.x, geo.width, str, 16, bg);

        if (self->selection_n != -1){
            if (self->selection_n == top_line+i){ // line with selection
                // generic case (both markers in same line)
                int idx1 = min(self->cursor_x, self->selection_x);
                int idx2 = max(self->cursor_x, self->selection_x);
                // if selection marker above
                if (self->selection_n < self->cursor_n){
                    idx1 = self->selection_x;
                    idx2 = current->length;
                }
                // if selection marker below
                if (self->cursor_n < self->selection_n){
                    idx1 = 0;
                    idx2 = self->selection_x;
                }
                int diff = idx2 - idx1;
                Buffer_print(&main_buf, geo.y + i, geo.x+idx1, diff, str+idx1, 16, 27);
            }
            if (self->cursor_n == top_line+i){ // line with cursor
                // generic case (both markers in same line)
                int idx1 = min(self->cursor_x, self->selection_x);
                int idx2 = max(self->cursor_x, self->selection_x);
                // if cursor above
                if (self->selection_n < self->cursor_n){
                    idx1 = 0;
                    idx2 = self->cursor_x;
                }
                // if cursor below
                if (self->cursor_n < self->selection_n){
                    idx1 = self->cursor_x;
                    idx2 = current->length;
                }
                int diff = idx2 - idx1;
                Buffer_print(&main_buf, geo.y + i, geo.x+idx1, diff, str+idx1, 16, 27);
            }
        }
        
        if (-self->win.shift + i == self->cursor_n){ // show cursor
            bg = 248;
            if (self->edit_mode == 1) bg = 3;
            Buffer_print(&main_buf, geo.y + i, geo.x+self->cursor_x, 1, str+self->cursor_x, 16, bg);
        }
        i++;
        if (current != NULL)
            current = current->next;
    }
}

EditorWindow *latestEditorWindow;
EditorWindow *EditorWindow_new()
{
    EditorWindow *self = malloc(sizeof *self);
    Window_init(self, -1, -1, -1, -1, -1, -1);
    self->win.top = 0;
    self->win.bottom = 0;
    self->win.left = 0;
    self->win.right = 0;
    self->win.id = "editor tab";
    self->top_n = 0;
    self->cursor_n = 0;
    self->cursor_x = 0;
    self->edit_mode = 0;
    self->selection_n = -1;
    self->selection_x = 0;

    // Window *editor = (Window *) self;
    self->win.draw = EditorWindow_draw;

    self->win.send_key = EditorWindow_send_key;

    self->win.scroll_wheel_up = EditorWindow_scroll_wheel_up;
    self->win.scroll_wheel_down = EditorWindow_scroll_wheel_down;

    latestEditorWindow = self;

    return self;
}

Window *EditorWindow_new_tab()
{
    EditorWindow *editor = EditorWindow_new();
    Window *slider = slider_new(editor);
    Slider_show_grip(slider);
    editor->slider = slider;

    editor->win.id = malloc(ID_LENGTH*4);
    slider->id = editor->win.id;

    return slider;
}

#include <ctype.h>
void replace_nonprintable(char *str)
{
    while (*str)
    {
        if (!isprint((unsigned char)*str))
        {
            *str = '?';
        }
        str++;
    }
}

Node *create_node(const char *text)
{
    Node *new_node = malloc(sizeof(Node));
    if (!new_node)
    {
        perror("malloc failed");
        exit(1);
    }
    replace_nonprintable(text);
    new_node->line = strdup(text); // copy string
    new_node->length = strlen(text);
    new_node->capacity = new_node->length + 1;
    new_node->next = NULL;
    new_node->prev = NULL;

    return new_node;
}

void append(EditorWindow *self, const char *text)
{
    Node *new_node = create_node(text);

    if (self->head == NULL)
    {
        self->head = new_node;
        // return;
    }
    if (self->tail != NULL)
        self->tail->next = new_node;
    new_node->prev = self->tail;
    self->tail = new_node;
}

#define MAX_LINE 10240

void load_file(EditorWindow *self, const char *filename)
{
    Window_set_id_from_path(self, "📝", filename);
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("fopen failed");
        return;
    }

    // Node *head = NULL;
    // Node *tail = NULL;
    char buffer[MAX_LINE];

    while (fgets(buffer, MAX_LINE, file))
    {
        // Optional: remove newline
        buffer[strcspn(buffer, "\n")] = '\0';

        append(self, buffer);
        self->n_lines++;
    }
    if (self->n_lines == 0){
        append(self, "");
        self->n_lines++;
    }

    fclose(file);

    self->win.virtual_height = self->n_lines;
    // return head;
}

void EditorWindow_open_file(EditorWindow *editor_window, char *file_path)
{
    load_file(editor_window, file_path);
    //editor_window->top = editor_window->head;
    editor_window->slider->id = file_path;
    //LOG_INFO("Editor_open_file %s", file_path);
    editor_window->file_path = strdup(file_path);
}

// Editor Frame

EditorFrame *last_frame;

Window *Editor_menu_new(EditorFrame *self)
{
    LOG_INFO("Editor_menu_new %p", self);
}

Window *Editor_menu_close(EditorFrame *self)
{
    LOG_INFO("Editor_menu_close %p", self);
}

Window *Editor_menu(EditorFrame *self)
{
    Window *menu = Menu_create_horizontal();

    Window *file = Menu_create_vertical(self);
    Menu_add_element(file, " 📄 New   Ctrl+N", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(file, " ❌ Close Ctrl+W", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(file, "", NULL);
    Menu_add_submenu(menu, " File ", file);

    Window *edit = Menu_create_vertical(self);
    Menu_add_element(edit, " ❌ Delete Backspace", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(edit, " 🔪 Cut    Ctrl+X", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(edit, " 📋 Copy   Ctrl+C", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(edit, " 📌 Paste  Ctrl+V", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(edit, "", NULL);
    Menu_add_submenu(menu, " Edit ", edit);

    Window *view = Menu_create_vertical(self);
    Menu_add_element(view, " ⤶ Word wrap", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(view, "", NULL);
    Menu_add_submenu(menu, " View ", view);

    Menu_add_windows(menu, " Window ", self->tabs->data, self);

    return menu;
}

Window *Editor_toolbar(EditorFrame *self)
{
    Window *toolbar = Menu_create_horizontal();
    Menu_add_element(toolbar, " 📄 New ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " ❌ Close ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " ❌ Delete ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " 🔪 Cut ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " 📋 Copy ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " 📌 Paste ", create_lambda(Editor_menu_new, 1, self));

    toolbar->top = 1;

    return toolbar;
}

Window *Editor_new(int left, int right, int top, int bottom, int width, int height)
{
    EditorFrame *editor_frame = malloc(sizeof *editor_frame);
    Window *frame = editor_frame;
    // Window *frame = malloc(sizeof *frame);
    Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 0);

    Window *tabs = Tab_new((Window * (*)(void)) EditorWindow_new_tab, 0);
    editor_frame->tabs = tabs;
    tabs->top = 2;
    tabs->bottom = 0;
    tabs->left = 0;
    tabs->right = 0;
    Window_append(w, tabs);
    frame->focused = tabs;

    Window *toolbar = Editor_toolbar(editor_frame);
    Window_append(w, toolbar);
    Window *menu = Editor_menu(editor_frame);
    Window_append(w, menu);

    last_frame = frame;

    return frame;
}

void Editor_open_file(EditorFrame *editor_frame, char *file_path)
{
    Window *slider = tabs_new_tab(editor_frame->tabs);
    // EditorWindow * editor_window = slider->focused;
    // EditorWindow * editor_window = tabs_new_tab(editor_frame->tabs);
    EditorWindow_open_file(latestEditorWindow, file_path);
}

void Editor_last_open_file(char *file_path)
{
    if (last_frame == NULL)
    {
        file_editor_new();
        // return;
    }
    // LOG_INFO("Editor_last_open_file %p", last_frame);
    Editor_open_file(last_frame, file_path);
    // Window_bring_to_bottom(last_frame);
    // root->focused = last_frame;
    TaskBar_switch_frame(last_frame);
}
