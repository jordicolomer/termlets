#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "text_edit.h"
#include "logger.h"
#include "buffer.h"

void LineEditorWindow_send_sequence(struct Window *win, const char *seq, int len){
  LOG_INFO("LineEditorWindow_send_sequence: %s", seq);
}

void delete_char(char *buffer, size_t pos, size_t len)
{
    if (pos >= len) return;                    // safety check
    
    // Shift everything after pos one position to the left
    memmove(buffer + pos, buffer + pos + 1, len - pos);
    
    // If it's a null-terminated string, update the length
    buffer[len - 1] = '\0';
}

void insert_char(char *buffer, size_t pos, char c, 
                 size_t current_len, size_t capacity)
{
    // Safety checks
    if (pos > current_len || current_len + 1 >= capacity) {
        return;  // Buffer full or invalid position
    }

    // Shift characters to the right to make space
    memmove(buffer + pos + 1, 
            buffer + pos, 
            current_len - pos + 1);  // +1 to include null terminator

    // Insert the new character
    buffer[pos] = c;
}

void LineEditorWindow_send_key(Window * win, char c){
    LineEditorWindow *self = win;
    int len = strlen(self->buffer);
    LOG_INFO("LineEditorWindow_send_key: %c %d", c, len);
    if (c == 8) { // Control+H
        if (self->cursor > 0){
            delete_char(self->buffer, self->cursor-1, len);
            self->cursor--;
        }
        return;
    }
    if (c == 4) { // Ctrl+D
        if (self->cursor > 0){
            self->cursor--;
        }
        return;
    }
    if (c == 6) { // Ctrl+F
        if (self->cursor < len){
            self->cursor++;
        }
        return;
    }
    insert_char(self->buffer, self->cursor, c, len, sizeof(self->buffer));
    self->cursor++;
}

void LineEditorWindow_draw(struct Window *current, int hasFocus)
{
    LineEditorWindow *self = current;
    Geometry geo = current->calculated;
    int fg = current->fg;
    int bg = current->bg;
    Buffer_print(&main_buf, geo.y, geo.x, geo.width, current->c, fg, bg);
    Buffer_print(&main_buf, geo.y, geo.x+self->cursor, 1, current->c+self->cursor, fg, 73);
}

LineEditorWindow * LineEditorWindow_new(char * c){
    LineEditorWindow *self = malloc(sizeof *self);
    Window_init(self, -1, -1, -1, -1, -1, -1);
    self->win.draw = LineEditorWindow_draw;

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