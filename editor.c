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

// Editor Window

void EditorWindow_make_cursor_visible(EditorWindow *self)
{
    int height = self->win.calculated.height;
    int diff = self->cursor_n + self->win.shift;
    if (diff < 0)
        self->win.shift = -(self->cursor_n);
    if (diff > height - 1)
        self->win.shift = height - 1 - self->cursor_n;
}

void EditorWindow_send_key(Window *win, char c)
{
    EditorWindow *self = win;
    if (c == 106)
    { // j
        self->cursor_n++;
        self->cursor_n = min(self->cursor_n, self->n_lines - 1);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 107)
    { // k
        self->cursor_n--;
        self->cursor_n = max(self->cursor_n, 0);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 117)
    { // u
        self->cursor_n += win->calculated.height;
        self->cursor_n = min(self->cursor_n, self->n_lines - 1);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (c == 105)
    { // i
        self->cursor_n -= win->calculated.height;
        self->cursor_n = max(self->cursor_n, 0);
        EditorWindow_make_cursor_visible(self);
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
    Node *current = self->head;
    for (int i = 0; i < -self->win.shift; i++)
        current = current->next;

    while (i < geo.height)
    {
        int bg = 255;
        if (-self->win.shift + i == self->cursor_n)
            bg = 27;
        if (bg >= 232 + 4 && !hasFocus)
            bg -= 4;

        char *str = "";
        if (current != NULL)
            str = current->line;
        Buffer_print(&main_buf, geo.y + i++, geo.x, geo.width, str, 16, bg);
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

    // Window *editor = (Window *) self;
    self->win.draw = EditorWindow_draw;

    self->win.send_key = EditorWindow_send_key;

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
    Window_set_id_from_path(self, filename);
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

    fclose(file);

    self->win.virtual_height = self->n_lines;
    // return head;
}

void EditorWindow_open_file(EditorWindow *editor_window, char *file_path)
{
    load_file(editor_window, file_path);
    editor_window->top = editor_window->head;
    editor_window->slider->id = file_path;
    LOG_INFO("Editor_open_file %s", file_path);
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
