#ifdef _WIN32
/* Disable strict pointer type warnings on Windows for this file */
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#endif

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
#include "mini_edit.h"
#include "clipboard.h"
#include "lexer.h"
#include "utils.h"
#include "common.h"
#include "config.h"
#include "mystr.h"
#include "collections.h"

// int restore_insert_mode;

void set_modified(EditorWindow *self, int modified) {
    if (modified == self->tab_window->tab.modified)
        return;
    self->tab_window->tab.modified = modified;
    Tab_set_title(self->tab_window, self->file_path);
}

// Editor Window
void EditorWindow_update_height(EditorWindow *self) {
    self->win.virtual_height = self->n_lines;
    Slider_show_grip(self->slider);
}

void EditorWindow_scroll_wheel_down(struct Window *w) {
    EditorWindow *self = w;
    Slider_scroll_down(self->slider);
}

void EditorWindow_scroll_wheel_up(struct Window *w) {
    EditorWindow *self = w;
    Slider_scroll_up(self->slider);
}

void EditorWindow_show_line(EditorWindow *self, int n) {
    int height = self->win.calculated.height;
    int diff = n + self->win.shift;
    if (diff < 0)
        self->win.shift = -(n);
    if (diff > height - 1)
        self->win.shift = height - 1 - n;
}

void EditorWindow_make_cursor_visible(EditorWindow *self) {
    EditorWindow_show_line(self, self->cursor.y);
}

Node *EditorWindow_get_line_number(EditorWindow *self, int number) {
    // todo: make this efficient
    Node *current = self->head;
    for (int i = 0; i < number; i++) {
        if (current == NULL)
            return current;
        current = current->next;
    }
    return current;
}

void double_capacity(Node *node) {
    size_t new_capacity = node->capacity * 2;

    char *new_line = realloc(node->line, new_capacity);
    if (new_line == NULL) {
        return; // or handle allocation failure
    }

    node->line = new_line;
    node->capacity = new_capacity;
}

void update_lexer_state(Node *current, int fast, int language) {
    /*
     * Run the lexer on all lines and save the lexer state on each line
     * so that we can run the lexer on a random line as if we processed each line before it
     */
    int lastLexerState = current->lexerState;
    int i = 0;
    while (current != NULL) {
        if (i > 0 && fast == 1 && current->lexerState == lastLexerState)
            return;
        current->lexerState = lastLexerState;
        Lexer lexer;
        Token token;
        token.start = 0;
        lexer_init(&lexer, current->line, language);
        lexer.state = lastLexerState;
        while (lexer_next(&lexer, &token)) {
        }
        lastLexerState = lexer.state;
        current = current->next;
        i++;
    }
}

void Node_append(Node *node, char *text) {
    if (!node || !text || *text == '\0')
        return;

    size_t text_len = strlen(text);
    // int width = calculate_width(text);

    // Check if we need to grow the buffer
    if (node->length + text_len + 1 > node->capacity) {
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
    // node->width += width;
    node->line[node->length] = '\0';
}

/*
void EditorWindow_delete_region(EditorWindow *self) {
    if (self->selection.y == -1)
        return;
    // get range
    int i1 = self->cursor.y;
    int j1_width = self->cursor.x; // display width
    int j1_ptr = self->cursor.ptr; // byte pointer
    int i2 = self->selection.y;
    int j2_width = self->selection.x; // display width
    int j2_ptr = self->selection.ptr; // byte pointer

    if (self->selection.y < self->cursor.y ||
        (self->selection.y == self->cursor.y && self->selection.x < self->cursor.x)) {
        i1 = self->selection.y;
        j1_width = self->selection.x;
        j1_ptr = self->selection.ptr;
        i2 = self->cursor.y;
        j2_width = self->cursor.x;
        j2_ptr = self->cursor.ptr;
    }

    Node *node1 = EditorWindow_get_line_number(self, i1);
    Node *node2 = EditorWindow_get_line_number(self, i2);

    node1->line[j1_ptr] = 0;
    node1->length = j1_ptr;  // byte length
    node1->width = j1_width; // display width
    Node_append(node1, node2->line + j2_ptr);

    node1->next = node2->next;
    if (node2->next == NULL) {
        self->tail = node1;
    } else {
        node2->next->prev = node1;
    }

    self->cursor.y = i1;
    self->cursor.x = j1_width;
    self->cursor.ptr = j1_ptr;

    self->selection.y = -1;
    self->n_lines -= i2 - i1;
    EditorWindow_update_height(self);

    update_lexer_state(node1, 1, self->language);
    set_modified(self, 1);
}
*/

void EditorWindow_save(EditorWindow *self) {
    FILE *file = fopen(self->file_path, "w");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    Node *current = self->head;
    while (current != NULL) {
        fputs(current->line, file);
        current = current->next;
        if (current != NULL)
            fputc('\n', file);
    }
    fclose(file);
    set_modified(self, 0);
}

void EditorWindow_fix_cursor_x(EditorWindow *self) {
    Node *node = EditorWindow_get_line_number(self, self->cursor.y);
    if (node == NULL)
        return;
    if (node->width < self->cursor.x) {
        self->cursor.x = node->width;
    }
    // Node * node = EditorWindow_get_line_number(self, self->cursor_n);
    if (self->cursor.x == node->width) {
        self->cursor.ptr = node->length;
    } else {
        self->cursor.ptr = char_at(node->line, self->cursor.x, &self->cursor.x) - node->line;
    }
}

Node *create_node(const char *text) {
    if (text == NULL) {
        text = "";
    }
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) {
        perror("malloc failed");
        exit(1);
    }
    new_node->line = strdup(text); // copy string
    new_node->length = strlen(text);
    // new_node->width = calculate_width(text);
    new_node->capacity = new_node->length + 1;
    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->lexerState = 0;

    return new_node;
}

void append(EditorWindow *self, const char *text) {
    if (self == NULL) {
        return;
    }
    Node *new_node = create_node(text);

    if (self->head == NULL) {
        self->head = new_node;
        // return;
    }
    if (self->tail != NULL) {
        self->tail->next = new_node;
    }
    new_node->prev = self->tail;
    self->tail = new_node;
}

#define MAX_LINE 10240

int file_is_utf8(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0;
    }

    char buffer[MAX_LINE];

    int line_count = 0;
    while (fgets(buffer, MAX_LINE, file)) {
        size_t pos = 0;
        size_t len = strlen(buffer);
        while (pos < len) {
            utf8proc_int32_t current;

            utf8proc_ssize_t bytes =
                utf8proc_iterate((const utf8proc_uint8_t *)buffer + pos, len - pos, &current);

            if (bytes < 0) {
                printf("Invalid UTF-8\n");
                fclose(file);
                return 0;
            }
            pos += bytes;
        }
    }
    fclose(file);
    return 1;
}

#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <ctype.h>

int hex_dump(char *dst, size_t dst_size, const char *src, size_t src_len) {
    const unsigned char *bytes = (const unsigned char *)src;
    size_t pos = 0;

    for (size_t i = 0; i < src_len; i += 16) {
        size_t n = src_len - i < 16 ? src_len - i : 16;

        int written = snprintf(dst + (pos < dst_size ? pos : dst_size),
                               pos < dst_size ? dst_size - pos : 0, "%08zX  ", i);

        if (written < 0)
            return -1;
        pos += (size_t)written;

        for (size_t j = 0; j < 16; j++) {
            if (j < n)
                written = snprintf(dst + (pos < dst_size ? pos : dst_size),
                                   pos < dst_size ? dst_size - pos : 0, "%02X ", bytes[i + j]);
            else
                written = snprintf(dst + (pos < dst_size ? pos : dst_size),
                                   pos < dst_size ? dst_size - pos : 0, "   ");

            if (written < 0)
                return -1;
            pos += (size_t)written;

            if (j == 7) {
                if (pos < dst_size && dst_size - pos > 1)
                    dst[pos] = ' ';
                pos++;
            }
        }

        written = snprintf(dst + (pos < dst_size ? pos : dst_size),
                           pos < dst_size ? dst_size - pos : 0, " |");

        if (written < 0)
            return -1;
        pos += (size_t)written;

        for (size_t j = 0; j < n; j++) {
            char c = isprint(bytes[i + j]) ? (char)bytes[i + j] : '.';

            if (pos < dst_size && dst_size - pos > 1)
                dst[pos] = c;
            pos++;
        }

        written = snprintf(dst + (pos < dst_size ? pos : dst_size),
                           pos < dst_size ? dst_size - pos : 0, "|\n");

        if (written < 0)
            return -1;
        pos += (size_t)written;
    }

    if (dst_size > 0)
        dst[pos < dst_size ? pos : dst_size - 1] = '\0';

    return pos > INT_MAX ? -1 : (int)pos;
}

void load_file_hex(EditorWindow *self, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen failed");
        return;
    }

    Tab_set_title(self->tab_window, filename);

    unsigned char buffer[16];
    char hex_line[256];
    size_t bytes_read;
    size_t offset = 0;

    while ((bytes_read = fread(buffer, 1, 16, file)) > 0) {
        hex_dump(hex_line, sizeof(hex_line), (const char *)buffer, bytes_read);

        // Remove trailing newline from hex_dump output
        size_t len = strlen(hex_line);
        if (len > 0 && hex_line[len - 1] == '\n') {
            hex_line[len - 1] = '\0';
        }

        append(self, hex_line);
        self->n_lines++;
        offset += bytes_read;
    }

    if (self->n_lines == 0) {
        append(self, "");
        self->n_lines++;
    }

    fclose(file);

    EditorWindow_update_height(self);

    self->cursor.ptr = 0;

    // init cursor
    if (self->head) {
        MyStr_init(&self->cursor.str_ite, self->head->line);
        MyStr_next_cluster(&self->cursor.str_ite);
    }
    self->cursor.node = self->head;
    self->cursor.y = 0;
}

void load_file(EditorWindow *self, const char *filename) {
    if (self == NULL) {
        return;
    }
    int is_utf8 = file_is_utf8(filename);
    if (!is_utf8) {
        load_file_hex(self, filename);
        return;
    }

    LOG_INFO("load_file %s", filename);
    Tab_set_title(self->tab_window, filename);
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen failed");
        return;
    }

    char buffer[MAX_LINE];

    int line_count = 0;
    while (fgets(buffer, MAX_LINE, file)) {
        line_count++;
        // Optional: remove newline
        buffer[strcspn(buffer, "\n")] = '\0';

        append(self, buffer);
        self->n_lines++;
    }
    if (self->n_lines == 0) {
        append(self, "");
        self->n_lines++;
    }

    fclose(file);

    EditorWindow_update_height(self);

    self->cursor.ptr = 0;

    // init cursor
    if (self->head) {
        MyStr_init(&self->cursor.str_ite, self->head->line);
        LOG_INFO("self->head->line %s", self->head->line);
        MyStr_next_cluster(&self->cursor.str_ite);
    }
    self->cursor.node = self->head;
    self->cursor.y = 0;

    ArrayList_init(&self->undo_stack, sizeof(UndoNode), 10);
}

void EditorWindow_run_lexer(EditorWindow *self) {
    update_lexer_state(self->head, 0, self->language);
}

void EditorWindow_open_file(EditorWindow *editor_window, char *file_path) {
    if (editor_window == NULL) {
        return;
    }
    load_file(editor_window, file_path);
    if (editor_window->slider != NULL) {
        editor_window->slider->id = file_path;
    }
    editor_window->file_path = strdup(file_path);
    if (ends_with_ignore_case(file_path, ".c"))
        editor_window->language = LANG_C;
    else if (ends_with_ignore_case(file_path, ".h"))
        editor_window->language = LANG_C;
    else if (ends_with_ignore_case(file_path, ".cpp"))
        editor_window->language = LANG_CPP;
    else if (ends_with_ignore_case(file_path, ".hpp"))
        editor_window->language = LANG_CPP;
    else if (ends_with_ignore_case(file_path, ".java"))
        editor_window->language = LANG_JAVA;
    else if (ends_with_ignore_case(file_path, ".js"))
        editor_window->language = LANG_JS;
    else if (ends_with_ignore_case(file_path, ".ts"))
        editor_window->language = LANG_TS;
    else if (ends_with_ignore_case(file_path, ".py"))
        editor_window->language = LANG_PY;
    EditorWindow_run_lexer(editor_window);
}

void EditorWindow_reload(EditorWindow *editor_window) {
    editor_window->head = NULL;
    editor_window->tail = NULL;
    editor_window->cursor.y = 0;
    editor_window->cursor.x = 0;
    editor_window->n_lines = 0;
    editor_window->selection.y = -1;
    EditorWindow_open_file(editor_window, editor_window->file_path);
}

void EditorWindow_start_selection(EditorWindow *self) {
    // self->selection.y = self->cursor.y;
    // self->selection.x = self->cursor.x;
    // self->selection.ptr = self->cursor.ptr;
    // memcpy(&b, &a, sizeof(a));
    self->selection = self->cursor;
}

int all_different(char *str, int size, char *query) {
    char *q = query;
    while (*q != NULL) {
        if (size == 1 && str[0] == *q)
            return 0;
        q++;
    }
    return 1;
}

int any_same(char *str, int size, char *query) {
    char *q = query;
    while (*q != NULL) {
        if (size == 1 && str[0] == *q)
            return 1;
        q++;
    }
    return 0;
}

// begin EditorPointer
// this should enclose the underlying data structure. every editor operation should be done through
// this class

void EditorPointer_insert(EditorWindow *self, EditorPointer *cursor, char *cb) {
    Node *current = cursor->node;
    Node *next_line = current->next;
    // Get the byte position for splitting the line
    size_t byte_pos = cursor->str_ite.cluster_start;

    char *tail = strdup(current->line + byte_pos);
    current->line[byte_pos] = '\0';
    current->length = byte_pos;                  // byte length
    current->width = cursor->str_ite.pos_column; // display width

    char *line = cb;
    int i = 0;
    while (line != NULL /* && *line != '\0'*/) {
        char *end = strchr(line, '\n');

        if (end != NULL) {
            *end = '\0'; // temporarily terminate this line
        }

        if (i == 0) { // first line
            Node_append(current, line);
        } else {
            Node *new_node = malloc(sizeof(Node));
            new_node->line = strdup(line);
            new_node->length = strlen(line);
            // new_node->width = calculate_width(line);
            new_node->capacity = new_node->length + 1;
            new_node->lexerState = 0;
            new_node->next = NULL; // Initialize to NULL to prevent garbage pointer
            current->next = new_node;
            new_node->prev = current;
            current = new_node;
            self->n_lines++;
        }
        if (end != NULL) {
            *end = '\n';
        }

        if (end == NULL || *line == '\0') {
            break;
        }

        line = end + 1;
        i += 1;
    }

    // Save byte length before appending tail
    size_t pos_before_tail = current->length;

    Node_append(current, tail);
    free(tail);

    // Update cursor position and iterator
    cursor->node = current;
    cursor->y += i;
    MyStr_init(&cursor->str_ite, current->line);

    // Move iterator to position where we were before appending tail
    size_t target_pos = pos_before_tail;
    while (self->cursor.str_ite.cluster_start < target_pos &&
           self->cursor.str_ite.cluster_start < current->length) {
        if (!MyStr_next_cluster(&cursor->str_ite))
            break;
    }

    current->next = next_line;
    if (next_line != NULL) {
        next_line->prev = current;
    } else {
        self->tail = current;
    }

    update_lexer_state(current, 1, self->language);
}

void EditorPointer_vertical_move(EditorPointer *ep, int is_up, int count, int keep_horizontal) {
    int pos_column = ep->str_ite.pos_column;
    for (int i = 0; i < count; i++) {
        Node *node = ep->node->next;
        if (is_up)
            node = ep->node->prev;
        if (node == NULL)
            break;
        ep->node = node;
        if (is_up)
            ep->y -= 1;
        else
            ep->y += 1;
    }
    MyStr_init(&ep->str_ite, ep->node->line);
    MyStr_next_cluster(&ep->str_ite);
    if (keep_horizontal)
        MyStr_get_grapheme_at_column(&ep->str_ite, pos_column);
}

int EditorPointer_is_beginning(EditorPointer *cursor) {
    return cursor->node->prev == NULL && cursor->str_ite.pos_cluster <= 0;
}

int EditorPointer_is_end(EditorPointer *cursor) {
    return cursor->node->next == NULL && MyStr_is_end(&cursor->str_ite);
}

void EditorPointer_right(EditorPointer *cursor) {
    if (MyStr_is_end(&cursor->str_ite)) { // if end of line
        EditorPointer_vertical_move(cursor, 0, 1, 0);
        // EditorPointer_down(&self->cursor);
    } else { // if not end of line
        MyStr_next_cluster(&cursor->str_ite);
    }
}

void EditorPointer_left(EditorPointer *cursor) {
    if (cursor->str_ite.pos_cluster <= 0) { // if beginning of line
        EditorPointer_vertical_move(cursor, 1, 1, 0);
        MyStr_get_last_grapheme(&cursor->str_ite);
    } else { // if not end of line
        MyStr_prev_cluster(&cursor->str_ite);
    }
}

int EditorPointer_search(EditorPointer *cursor, char *query) {
    Node *node = cursor->node;
    while (node != NULL) {
        char *str = node->line;
        int start = 0;
        if (node == cursor->node) {
            start = cursor->str_ite.cluster_start;
        }
        char *found = strstr(str + start, query);
        if (found != NULL) {
            int position = found - str;
            cursor->node = node;
            MyStr_init(&cursor->str_ite, str);
            MyStr_get_grapheme_at_idx(&cursor->str_ite, position);
            return 1;
        }
        node = node->next;
        cursor->y++;
    }
    return 0;
}

void EditorPointer_next_char(EditorPointer *cursor, char *query) {
    int state = 0;
    while (!EditorPointer_is_end(cursor)) {
        EditorPointer_right(cursor);

        char *str = cursor->str_ite.utf8 + cursor->str_ite.cluster_start;
        int size = size;

        if (state == 0) {
            if (all_different(str, size, query)) {
                state = 1;
            }
        } else if (state == 1) {
            if (any_same(str, size, query)) {
                return;
            }
        }
    }
}

void EditorPointer_prev_char(EditorPointer *cursor, char *query) {
    int state = 0;
    while (!EditorPointer_is_beginning(cursor)) {
        EditorPointer_left(cursor);

        char *str = cursor->str_ite.utf8 + cursor->str_ite.cluster_start;
        int size = size;

        if (state == 0) {
            if (all_different(str, size, query)) {
                state = 1;
            }
        } else if (state == 1) {
            if (any_same(str, size, query)) {
                return;
            }
        }
    }
}

int EditorPointer_cmp(EditorPointer *ep1, EditorPointer *ep2) {
    if (ep1->y < ep2->y)
        return -1;
    if (ep1->y > ep2->y)
        return 1;
    if (ep1->str_ite.cluster_start < ep2->str_ite.cluster_start)
        return -1;
    if (ep1->str_ite.cluster_start > ep2->str_ite.cluster_start)
        return 1;
    return 0;
}

// return size in bytes required to store the region in memory
// ep1 comes before ep2
int EditorPointer_region_size(EditorPointer *ep1, EditorPointer *ep2) {
    if (ep1->node == ep2->node) {
        // Same line selection
        return ep2->str_ite.cluster_start - ep1->str_ite.cluster_start + 1;
    } else {
        // Multi-line selection
        int size = 1;                                           // ending NULL value
        size += ep1->node->length - ep1->str_ite.cluster_start; // first line
        size += ep2->str_ite.cluster_start;                     // last line
        size += ep2->y - ep1->y;                                // newline characters

        Node *node = ep1->node->next;
        while (node != NULL && node != ep2->node) {
            size += node->length;
            node = node->next;
        }
        return size;
    }
}

char *EditorPointer_copy_region_to_memory(EditorPointer *ep1, EditorPointer *ep2) {
    int cmp = EditorPointer_cmp(ep1, ep2);
    if (cmp == 0)
        return NULL;
    if (cmp == 1) {
        EditorPointer *tmp = ep1;
        ep1 = ep2;
        ep2 = tmp;
    }

    int size = EditorPointer_region_size(ep1, ep2);
    if (size == 0)
        return NULL;
    char *buf = (char *)malloc(size);

    char *dst = buf;
    Node *node = ep1->node;

    if (ep1->node == ep2->node) {
        // Same line selection
        int len = ep2->str_ite.cluster_start - ep1->str_ite.cluster_start;
        memcpy(dst, node->line + ep1->str_ite.cluster_start, len);
        dst += len;
    } else {
        // Multi-line selection
        // Copy first line from start position to end
        int first_len = node->length - ep1->str_ite.cluster_start;
        memcpy(dst, node->line + ep1->str_ite.cluster_start, first_len);
        dst += first_len;
        node = node->next;

        // Copy intermediate lines, including newline characters.
        while (node != NULL && node != ep2->node) {
            *dst++ = '\n';
            memcpy(dst, node->line, node->length);
            dst += node->length;

            node = node->next;
        }

        // Copy from the beginning of the last line to the ending position.
        if (node != NULL) {
            *dst++ = '\n';
            int last_len = ep2->str_ite.cluster_start;
            memcpy(dst, node->line, last_len);
            dst += last_len;
        }
    }

    *dst = '\0';
    return buf;
}

void EditorPointer_delete_region(EditorWindow *self, EditorPointer *ep1, EditorPointer *ep2) {
    int cmp = EditorPointer_cmp(ep1, ep2);
    if (cmp == 0)
        return;
    if (cmp == 1) {
        EditorPointer *tmp = ep1;
        ep1 = ep2;
        ep2 = tmp;
    }

    Node *node1 = ep1->node;
    Node *node2 = ep2->node;

    // Save the column position before deletion
    int target_column = ep1->str_ite.pos_column;

    int j1_ptr = ep1->str_ite.cluster_start; // byte position, not cluster number
    int j2_ptr = ep2->str_ite.cluster_start; // byte position, not cluster number

    if (node1 == node2) {
        // Same line deletion - use memmove to handle overlapping memory
        size_t bytes_to_delete = j2_ptr - j1_ptr;

        size_t remaining_bytes = node1->length - j2_ptr;

        memmove(node1->line + j1_ptr, node1->line + j2_ptr, remaining_bytes);
        node1->length -= bytes_to_delete;
        node1->line[node1->length] = '\0';

        // Recalculate width for the entire line
        // node1->width = calculate_width(node1->line);
    } else {
        // Multi-line deletion
        node1->line[j1_ptr] = 0;
        node1->length = j1_ptr; // byte length
        Node_append(node1, node2->line + j2_ptr);

        // update line count
        self->n_lines -= ep2->y - ep1->y;

        // Update linked list - remove intermediate nodes
        node1->next = node2->next;
        if (node2->next == NULL) {
            self->tail = node1;
        } else {
            node2->next->prev = node1;
        }

        // TODO: Free intermediate nodes between node1 and node2
    }

    // Reinitialize the iterator for the modified line and position at target column
    MyStr_init(&ep1->str_ite, node1->line);
    MyStr_next_cluster(&ep1->str_ite);
	//LOG_INFO("MyStr_get_grapheme_at_column %d", target_column);
    MyStr_get_grapheme_at_column(&ep1->str_ite, target_column);

    // Update node reference
    ep1->node = node1;

    // merge pointers
    *ep2 = *ep1;

    update_lexer_state(node1, 1, self->language);
}
// end EditorPointer

// start undo
void EditorPointer_undo_insert(EditorWindow *self, EditorPointer *cursor, char *cb) {
    ArrayList *stack = &self->undo_stack;
	//LOG_INFO("EditorPointer_undo_insert %d", cursor->str_ite.cluster_start);
    ArrayList_append(stack, (char *)&(UndoNode){.str = strdup(cb),
                                                .y = cursor->y,
                                                .ptr = cursor->str_ite.cluster_start,
                                                .is_insert = 0});
    self->undo_pointer = stack->last;
    EditorPointer_insert(self, cursor, cb);
}

void EditorPointer_undo_delete_region(EditorWindow *self, EditorPointer *ep1, EditorPointer *ep2) {
    int cmp = EditorPointer_cmp(ep1, ep2);
    if (cmp == 0)
        return;
    if (cmp == 1) {
        EditorPointer *tmp = ep1;
        ep1 = ep2;
        ep2 = tmp;
    }

    char *cb = EditorPointer_copy_region_to_memory(ep1, ep2);
    ArrayList *stack = &self->undo_stack;
    ArrayList_append(
        stack,
        (char *)&(UndoNode){.str = cb, .y = ep1->y, .ptr = ep1->str_ite.cluster_start, .is_insert = 1});
    self->undo_pointer = stack->last;
    EditorPointer_delete_region(self, ep1, ep2);
}

EditorPointer get_cursor(EditorWindow *self, int y, int ptr) {
    EditorPointer pointer;
    Node *node = EditorWindow_get_line_number(self, y);

    MyStr_init(&pointer.str_ite, node->line);
    MyStr_get_grapheme_at_idx(&pointer.str_ite, ptr);
    pointer.node = node;
    pointer.y = y;
    return pointer;
}

void EditorPointer_advance_bytes(EditorPointer *cursor, int n_bytes) {
  while (n_bytes > 0){
	EditorPointer_right(cursor);
	n_bytes -= cursor->str_ite.width_bytes;
  }
}

void EditorPointer_delete_region_bytes(EditorWindow *self, EditorPointer *ep1, int n_bytes) {
  EditorPointer ep2 = *ep1;
  EditorPointer_advance_bytes(&ep2, n_bytes);
  EditorPointer_delete_region(self, ep1, &ep2);
}

void EditorWindow_undo(EditorWindow *self) {
    /*LOG_INFO("EditorWindow_undo");
    ArrayListIterator ite = stack->first;
    while (ArrayListIteratorValid(stack, &ite)){
          UndoNode * undo_node = ArrayListIteratorElement(stack, &ite);
          if (undo_node != NULL){
            LOG_INFO("EditorWindow_undo %s %d %d %d", undo_node->str, undo_node->y, undo_node->ptr,
    undo_node->is_insert);
          }
          ArrayListIteratorNext(stack, &ite);
    }
    ite = stack->last;
    UndoNode * undo_node = ArrayListIteratorElement(stack, &ite);
    if (undo_node != NULL){
          LOG_INFO("EditorWindow_undo (last) %s %d %d %d", undo_node->str, undo_node->y,
    undo_node->ptr, undo_node->is_insert);
          }*/
    ArrayList *stack = &self->undo_stack;
	if (!ArrayListIteratorValid (stack, &self->undo_pointer) && self->undo_pointer.ar != NULL) {
	  self->undo_pointer = stack->last;
	}
    if (ArrayListIteratorValid (stack, &self->undo_pointer)) {
        UndoNode *undo_node = ArrayListIteratorElement(stack, &self->undo_pointer);
        if (undo_node != NULL) {
		    EditorPointer cursor = get_cursor(self, undo_node->y, undo_node->ptr);
			int n_bytes = strlen(undo_node->str);
			LOG_INFO("EditorWindow_undo %s %d %d %d %d", undo_node->str, n_bytes, undo_node->y, undo_node->ptr, undo_node->is_insert);
            if (undo_node->is_insert == 1) {
                EditorPointer_insert(self, &cursor, undo_node->str);
            } else {
				EditorPointer_delete_region_bytes(self, &cursor, n_bytes);
			}
			self->cursor = get_cursor(self, undo_node->y, undo_node->ptr);
			EditorWindow_make_cursor_visible(self);
        }
        ArrayListIteratorPrev(stack, &self->undo_pointer);
    }
}

void EditorWindow_redo(EditorWindow *self) {
    ArrayList *stack = &self->undo_stack;

	// if before first, init to first
	if (self->undo_pointer.ar == NULL){
	  self->undo_pointer.ar = stack->first_node;
	  self->undo_pointer.idx = 0;
	}
    if (ArrayListIteratorValid (stack, &self->undo_pointer)) {
        UndoNode *undo_node = ArrayListIteratorElement(stack, &self->undo_pointer);
		int n_bytes = strlen(undo_node->str);
		LOG_INFO("EditorWindow_redo %s %d %d %d %d", undo_node->str, n_bytes, undo_node->y, undo_node->ptr, undo_node->is_insert);
        if (undo_node != NULL) {
		    EditorPointer cursor = get_cursor(self, undo_node->y, undo_node->ptr);
            if (undo_node->is_insert == 0) {
                EditorPointer_insert(self, &cursor, undo_node->str);
            } else {
				EditorPointer_delete_region_bytes(self, &cursor, n_bytes);
			}
			self->cursor = get_cursor(self, undo_node->y, undo_node->ptr);
			EditorWindow_make_cursor_visible(self);
        }
        ArrayListIteratorNext(stack, &self->undo_pointer);
    }
}
// end undo

// EditorWindow

void EditorWindow_paste(EditorWindow *self) {
    char *cb = clipboard_paste();

    EditorPointer_undo_insert(self, &self->cursor, cb);

    // Check if clipboard is empty
    if (cb == NULL || *cb == '\0') {
        if (cb)
            free(cb);
        return;
    }
    set_modified(self, 1);

    EditorWindow_update_height(self);
    EditorWindow_make_cursor_visible(self);

    free(cb);
}

void EditorWindow_insert(EditorWindow *self, char c) {
    set_modified(self, 1);
    char buf[2];
    buf[0] = c;
    buf[1] = 0;
    EditorPointer_undo_insert(self, &self->cursor, buf);
}

void EditorWindow_newline(EditorWindow *self) { EditorWindow_insert(self, '\n'); }

void _EditorWindow_copy(EditorWindow *self) {
    char *buffer = EditorPointer_copy_region_to_memory(&self->cursor, &self->selection);
    if (buffer != NULL) {
        clipboard_copy(buffer);
        free(buffer);
    }
}

void EditorWindow_copy(EditorWindow *self) {
    _EditorWindow_copy(self);
    self->selection.y = -1;
}

void EditorWindow_cut(EditorWindow *self) {
    _EditorWindow_copy(self);
    EditorPointer_undo_delete_region(self, &self->cursor, &self->selection);

    EditorWindow_update_height(self);
    set_modified(self, 1);
    self->selection.y = -1;
}

void EditorWindow_first_line(EditorWindow *self) {
    if (self->head) {
        MyStr_init(&self->cursor.str_ite, self->head->line);
        MyStr_next_cluster(&self->cursor.str_ite);
    }
    self->cursor.node = self->head;
    self->cursor.y = 0;
}

void EditorWindow_last_line(EditorWindow *self) {
    if (self->tail) {
        MyStr_init(&self->cursor.str_ite, self->tail->line);
        MyStr_next_cluster(&self->cursor.str_ite);
    }
    self->cursor.node = self->tail;
    self->cursor.y = self->n_lines - 1;
}

void EditorWindow_action_start_of_line(EditorWindow *self) {
    MyStr_get_grapheme_at_column(&self->cursor.str_ite, 0);
}

void EditorWindow_action_end_of_line(EditorWindow *self) {
    MyStr_get_last_grapheme(&self->cursor.str_ite);
}

void _EditorWindow_up(EditorWindow *self) { EditorPointer_vertical_move(&self->cursor, 1, 1, 1); }

void _EditorWindow_down(EditorWindow *self) {
    int pos_column = self->cursor.str_ite.pos_column;
    EditorPointer_vertical_move(&self->cursor, 0, 1, 1);
}

void _EditorWindow_right(EditorWindow *self) { EditorPointer_right(&self->cursor); }

void _EditorWindow_left(EditorWindow *self) { EditorPointer_left(&self->cursor); }

void EditorWindow_up(EditorWindow *self) {
    _EditorWindow_up(self);
    if (insert_mode == 1) {
        EditorWindow_start_selection(self);
    }
}

void EditorWindow_down(EditorWindow *self) {
    _EditorWindow_down(self);
    if (insert_mode == 1) {
        EditorWindow_start_selection(self);
    }
}

void EditorWindow_right(EditorWindow *self) {
    _EditorWindow_right(self);
    if (insert_mode == 1) {
        EditorWindow_start_selection(self);
    }
}

void EditorWindow_left(EditorWindow *self) {
    _EditorWindow_left(self);
    if (insert_mode == 1) {
        EditorWindow_start_selection(self);
    }
}

void EditorWindow_shift_up(EditorWindow *self) { _EditorWindow_up(self); }

void EditorWindow_shift_down(EditorWindow *self) { _EditorWindow_down(self); }

void EditorWindow_shift_right(EditorWindow *self) { _EditorWindow_right(self); }

void EditorWindow_shift_left(EditorWindow *self) { _EditorWindow_left(self); }

char *next_word_chars = " \t{}()";

void EditorWindow_next_word(EditorWindow *self) {
    EditorPointer_next_char(&self->cursor, next_word_chars);
}

void EditorWindow_prev_word(EditorWindow *self) {
    EditorPointer_prev_char(&self->cursor, next_word_chars);
}

void EditorWindow_backspace(EditorWindow *self) {
    set_modified(self, 1);
    EditorPointer pointer;
    pointer = self->cursor;
    EditorPointer_left(&pointer);
    EditorPointer_undo_delete_region(self, &self->cursor, &pointer);
}

void EditorWindow_delete_word(EditorWindow *self) {
    set_modified(self, 1);
    EditorPointer pointer;
    pointer = self->cursor;
    EditorPointer_prev_char(&pointer, next_word_chars);
    EditorPointer_undo_delete_region(self, &self->cursor, &pointer);
}

void EditorWindow_delete(EditorWindow *self) {
    set_modified(self, 1);
    EditorPointer pointer;
    pointer = self->cursor;
    EditorPointer_right(&pointer);
    EditorPointer_undo_delete_region(self, &self->cursor, &pointer);
}

void EditorWindow_send_sequence(Window *win, const char *seq, int len) {
    EditorWindow *self = win;
    if (strlen(seq) == 0) {
        insert_mode = 0;
        return;
    }
    if (strcmp(seq, "[A") == 0) {
        EditorWindow_up(self);
        return;
    }
    if (strcmp(seq, "[B") == 0) {
        EditorWindow_down(self);
        return;
    }
    if (strcmp(seq, "[C") == 0) {
        EditorWindow_right(self);
        return;
    }
    if (strcmp(seq, "[D") == 0) {
        EditorWindow_left(self);
        return;
    }
    if (strcmp(seq, "[1;2A") == 0) {
        EditorWindow_shift_up(self);
        return;
    }
    if (strcmp(seq, "[1;2B") == 0) {
        EditorWindow_shift_down(self);
        return;
    }
    if (strcmp(seq, "[1;2C") == 0) {
        EditorWindow_shift_right(self);
        return;
    }
    if (strcmp(seq, "[1;2D") == 0) {
        EditorWindow_shift_left(self);
        return;
    }
    if (strcmp(seq, "[1;3D") == 0) {
        EditorWindow_prev_word(self);
        return;
    }
    if (strcmp(seq, "[1;3C") == 0) {
        EditorWindow_next_word(self);
        return;
    }
}

int count_tabs(EditorWindow *self, Node *node) {
    int tabs = 0;
    while (tabs < node->length) {
        if (node->line[tabs] != '\t')
            break;
        tabs++;
    }
    return tabs;
}

int count_leading_tabs_or_spaces(EditorWindow *self, Node *node) {
    int tabs = 0;
    while (tabs < node->length) {
        if (node->line[tabs] != '\t' && node->line[tabs] != ' ')
            break;
        tabs++;
    }
    return tabs;
}

int is_empty(Node *node) {
    int tabs = 0;
    while (tabs < node->length) {
        if (node->line[tabs] != '\t' && node->line[tabs] != ' ')
            return 0;
        tabs++;
    }
    return 1;
}

Node *get_previous_line(Node *node) {
    Node *current = node->prev;
    while (current != NULL && is_empty(current)) {
        current = current->prev;
    }
    return current;
}

void EditorWindow_action_indent(EditorWindow *self) {
    int open_bracket = 0;
    int close_bracket = 0;
    int prev_tabs = 0;
    Node *current_node = self->cursor.node;
    int current_tabs = count_tabs(self, current_node);
    close_bracket = current_node->line[strlen(current_node->line) - 1] == '}';

    Node *prev_node = get_previous_line(current_node);
    if (prev_node != NULL) {
        prev_tabs = count_tabs(self, prev_node);
        open_bracket = prev_node->line[strlen(prev_node->line) - 1] == '{';
    }

    int leading_tabs_or_spaces = count_leading_tabs_or_spaces(self, current_node);
    if (leading_tabs_or_spaces > 0) {
        // Create a pointer at end of leading whitespace
        EditorPointer end_ptr = self->cursor;
        MyStr_init(&end_ptr.str_ite, current_node->line);
        MyStr_next_cluster(&end_ptr.str_ite);
        MyStr_get_grapheme_at_idx(&end_ptr.str_ite, leading_tabs_or_spaces);

        // Create a pointer at start of line
        EditorPointer start_ptr = self->cursor;
        MyStr_init(&start_ptr.str_ite, current_node->line);
        MyStr_next_cluster(&start_ptr.str_ite);

        EditorPointer_undo_delete_region(self, &end_ptr, &start_ptr);

        // Update cursor to the start position after deletion
        self->cursor = start_ptr;
    }

    if (is_empty(current_node))
        return;

    EditorWindow_action_start_of_line(self);
    int diff = prev_tabs + open_bracket - close_bracket;
    for (int i = 0; i < diff; i++) {
        EditorWindow_insert(self, '\t');
    }
}

// start of search
/*
void _EditorWindow_search(EditorWindow *self, char *query) {
    EditorPointer ptr = self->highlight_end;

    if (ptr.y == -1) {
        ptr = self->cursor;
    }

    int n = ptr.y;

    Node *node = EditorWindow_get_line_number(self, n);
    char *p = strstr(node->line + ptr.ptr, query);
    while (p == NULL && n < self->n_lines - 1) {
        n += 1;
        node = EditorWindow_get_line_number(self, n);
        p = strstr(node->line, query);
    }

    if (p != NULL) {
        self->highlight_start.y = n;
        //self->highlight_start.x = calculate_width_n(node->line, p - node->line);
        self->highlight_start.ptr = p - node->line;

        self->highlight_end.y = n;
        self->highlight_end.x = self->highlight_start.x + strlen(query);
        self->highlight_end.ptr = self->highlight_start.x + calculate_width(query);

        EditorWindow_show_line(self, self->highlight_end.y);
    }
}
*/

void EditorWindow_search(EditorWindow *self, char *query) {
    EditorPointer ptr = self->highlight_end;

    if (ptr.y == -1) {
        ptr = self->cursor;
    }
    // EditorPointer ptr = self->cursor;
    if (EditorPointer_search(&ptr, query)) {
        self->highlight_start = ptr;
        self->highlight_end = ptr;
        MyStr_get_grapheme_at_idx(&self->highlight_end.str_ite,
                                  self->highlight_end.str_ite.cluster_start + strlen(query));
        EditorWindow_show_line(self, ptr.y);
    }
}

void EditorWindow_searchbox_exit(EditorWindow *self) {
    self->search_box->win.hidden = 1;
    search_mode = 0;
    self->win.focused = NULL;
    self->highlight_start.y = -1;
}

void EditorWindow_searchbox_on_modify(EditorWindow *self) {
    self->highlight_end.y = -1;
    EditorWindow_search(self, self->search_box->buffer);
}

void Editor_searchbox_on_enter(EditorWindow *self) {
    self->search_box->win.hidden = 1;
    search_mode = 0;
    self->win.focused = NULL;
    if (self->highlight_end.y != -1) {
        self->cursor = self->highlight_end;
        self->highlight_start.y = -1;
    }
    // EditorWindow_show_line(self, self->cursor.y);
    EditorWindow_make_cursor_visible(self);
}

Window *EditorWindow_searchbox(EditorWindow *self) {
    LineEditorWindow *line_edit = LineEditorWindow_new(NULL, "Search");
    line_edit->win.top = 1;
    line_edit->win.bottom = -1;
    line_edit->win.left = -1;
    line_edit->win.right = 3;
    line_edit->win.width = 15;
    line_edit->win.height = 1;
    line_edit->win.bg = 229;
    line_edit->win.fg = 16;
    line_edit->win.hidden = 1;
    line_edit->win.id = "EditorWindow_searchbox";
    line_edit->on_exit = create_lambda(EditorWindow_searchbox_exit, 1, self);
    line_edit->on_modify = create_lambda(EditorWindow_searchbox_on_modify, 1, self);
    line_edit->win.lambda = create_lambda(Editor_searchbox_on_enter, 1, self);
    return line_edit;
}

void EditorWindow_action_search(EditorWindow *self) {
    if (self->search_box->win.hidden == 1) {
        self->search_box->win.hidden = 0;
        search_mode = 1;
        self->win.focused = self->search_box;
        LineEditorWindow_reset(self->search_box);
    } else {
        EditorWindow_search(self, self->search_box->buffer);
    }
}

// end of search

void EditorWindow_send_key(Window *win, char c) {

    EditorWindow *self = win;
    Action action = get_action(c, WT_EDITOR);

    if (action == ACTION_SEARCH) { // todo
        EditorWindow_action_search(self);
        return;
    }

    Window *focused_cursor = win->focused;
    if (focused_cursor != NULL)
        while (focused_cursor->send_key == NULL && focused_cursor->focused != NULL)
            focused_cursor = focused_cursor->focused;

    if (focused_cursor != NULL && focused_cursor->send_key != NULL) {
        focused_cursor->send_key(focused_cursor, c);
        return;
    }

    if (action == ACTION_INDENT) { // todo
        EditorWindow_action_indent(self);
        return;
    }
    if (action == ACTION_BACKSPACE) {
        EditorWindow_backspace(self);
        self->selection.y = -1;
        return;
    }
    if (action == ACTION_ENTER) {
        EditorWindow_newline(self);
        return;
    }
    if (action == ACTION_START_OF_LINE) {
        EditorWindow_action_start_of_line(self);
        return;
    }
    if (action == ACTION_INSERT) {
        insert_mode = 1;
        return;
    }
    if (action == ACTION_LEFT) {
        EditorWindow_left(self);
        return;
    }
    if (action == ACTION_RIGHT) {
        EditorWindow_right(self);
        return;
    }
    if (action == ACTION_NEXT_WORD) {
        EditorWindow_next_word(self);
        return;
    }
    if (action == ACTION_PREV_WORD) {
        EditorWindow_prev_word(self);
        return;
    }
    if (action == ACTION_END_OF_LINE) {
        // Node *node = EditorWindow_get_line_number(self, self->cursor.y);
        // int mx = node->width;
        // self->cursor.x = mx;
        // EditorWindow_fix_cursor_x(self);
        EditorWindow_action_end_of_line(self);
        return;
    }
    if (action == ACTION_DOWN) {
        EditorWindow_down(self);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (action == ACTION_FIRST_LINE) {
        // self->cursor.y = self->n_lines - 1;
        // EditorWindow_fix_cursor_x(self);
        EditorWindow_first_line(self);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (action == ACTION_UP) {
        EditorWindow_up(self);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (action == ACTION_LAST_LINE) {
        EditorWindow_last_line(self);
        // self->cursor.y = 0;
        // EditorWindow_fix_cursor_x(self);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (action == ACTION_PAGE_UP) {
        // self->cursor.y += win->calculated.height;
        // self->cursor.y = min(self->cursor.y, self->n_lines - 1);
        EditorPointer_vertical_move(&self->cursor, 1, win->calculated.height, 1);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (action == ACTION_PAGE_DOWN) {
        // self->cursor.y -= win->calculated.height;
        // self->cursor.y = max(self->cursor.y, 0);
        EditorPointer_vertical_move(&self->cursor, 0, win->calculated.height, 1);
        EditorWindow_make_cursor_visible(self);
        return;
    }
    if (action == ACTION_START_SELECTION) {
        EditorWindow_start_selection(self);
        return;
    }
    if (action == ACTION_COPY) {
        EditorWindow_copy(self);
        return;
    }
    if (action == ACTION_PASTE) {
        EditorWindow_paste(self);
        return;
    }
    if (action == ACTION_CUT) {
        EditorWindow_cut(self);
        return;
    }
    if (action == ACTION_SAVE) {
        EditorWindow_save(self);
        return;
    }
    if (action == ACTION_RELOAD) {
        EditorWindow_reload(self);
        return;
    }
    if (action == ACTION_INSERT_SEMICOLON) {
        self->selection.y = -1;
        EditorWindow_insert(self, ';');
        return;
    }
    if (action == ACTION_DELETE) {
        EditorWindow_delete(self);
        return;
    }
    if (action == ACTION_DELETE_WORD) {
        EditorWindow_delete_word(self);
        return;
    }
    if (action == ACTION_UNDO) {
        EditorWindow_undo(self);
        return;
    }
    if (action == ACTION_REDO) {
        EditorWindow_redo(self);
        return;
    }
    if ((c > 31 || c == '\t')) {
        self->selection.y = -1;
        EditorWindow_insert(self, c);
        return;
    }
}

void EditorWindow_draw_selection(struct Window *w, EditorPointer p1, EditorPointer p2, int color) {
    if (p1.y == -1)
        return;
    if (p2.y == -1)
        return;
    EditorWindow *self = w;
    Geometry geo = w->calculated;
    int i = 0;
    int top_line = -self->win.shift;
    while (i < geo.height) {
        if (p2.y != -1) {
            if ((p1.y < top_line + i && top_line + i < p2.y) ||
                (p2.y < top_line + i && top_line + i < p1.y)) {
                Buffer_set_bg(&main_buf, geo.y + i, geo.x + 0, geo.width, color);
            }
            if (p2.y == top_line + i) { // line with selection
                // generic case (both markers in same line)
                int idx1 = min(p1.str_ite.pos_column, p2.str_ite.pos_column);
                int idx2 = max(p1.str_ite.pos_column, p2.str_ite.pos_column);
                // if selection marker above
                if (p2.y < p1.y) {
                    idx1 = p2.str_ite.pos_column;
                    idx2 = geo.width;
                }
                // if selection marker below
                if (p1.y < p2.y) {
                    idx1 = 0;
                    idx2 = p2.str_ite.pos_column;
                }
                int diff = idx2 - idx1;
                Buffer_set_bg(&main_buf, geo.y + i, geo.x + idx1, diff, color);
            }
            if (p1.y == top_line + i) { // line with cursor
                // generic case (both markers in same line)
                int idx1 = min(p1.str_ite.pos_column, p2.str_ite.pos_column);
                int idx2 = max(p1.str_ite.pos_column, p2.str_ite.pos_column);
                // if cursor above
                if (p2.y < p1.y) {
                    idx1 = 0;
                    idx2 = p1.str_ite.pos_column;
                }
                // if cursor below
                if (p1.y < p2.y) {
                    idx1 = p1.str_ite.pos_column;
                    idx2 = geo.width;
                }
                int diff = idx2 - idx1;
                Buffer_set_bg(&main_buf, geo.y + i, geo.x + idx1, diff, color);
            }
        }
        i++;
    }
}

void EditorWindow_draw(struct Window *w, int hasFocus) {
    EditorWindow *self = w;

    Geometry geo = w->calculated;
    int i = 0;
    int top_line = -self->win.shift;
    Node *current = EditorWindow_get_line_number(self, top_line);

    while (i < geo.height) {
        // background color
        int bg = 255;
        if (bg >= 232 + 2 && !hasFocus)
            bg -= 2;

        char *str = "";
        if (current != NULL)
            str = current->line;

        // for(int x=0;x<geo.width;x++) Buffer_print(&main_buf, geo.y + i, geo.x+x, 1, " ", 16, bg);
        Buffer_print(&main_buf, geo.y + i, geo.x, geo.width, str, 16, bg);

        // syntax highlighter

        if (self->language > LANG_NONE) {
            Lexer lexer;
            Token token;
            lexer_init(&lexer, str, self->language);
            if (current != NULL)
                lexer.state = current->lexerState;
            while (lexer_next(&lexer, &token)) {
                int width = token.end_screen - token.start_screen;
                int maxWidth = geo.width - token.start;
                width = min(width, maxWidth);
                Buffer_set_fg(&main_buf, geo.y + i, geo.x + token.start_screen, width, token.color);
            }
        }

        i++;
        if (current != NULL)
            current = current->next;
    }
    EditorWindow_draw_selection(w, self->cursor, self->selection, 117);
    EditorWindow_draw_selection(w, self->highlight_start, self->highlight_end, 227);

    // draw cursor
    int bg = 208;
    if (insert_mode == 1)
        bg = 27;
    if (hasFocus == 0)
        bg = 248;
    i = self->cursor.y + self->win.shift;
    if (0 <= i && i < geo.height)
        Buffer_set_bg(&main_buf, geo.y + i, geo.x + self->cursor.str_ite.pos_column, 1, bg);
}

void EditorWindow_on_mouse_down(Window *win, int x, int y) {
    EditorFrame *frame = Window_get_frame(win);

    EditorWindow *self = win;

    // Calculate target line
    int target_y = y - win->shift - win->calculated.y;
    target_y = min(target_y, self->n_lines - 1);
    target_y = max(target_y, 0);

    // Calculate target column
    int target_x = x - win->calculated.x;
    target_x = max(target_x, 0);

    // Get the node for the target line
    Node *node = EditorWindow_get_line_number(self, target_y);
    if (node == NULL)
        return;

    // Update cursor position
    self->cursor.y = target_y;
    self->cursor.node = node;

    // Initialize iterator and position at target column
    MyStr_init(&self->cursor.str_ite, node->line);
    MyStr_next_cluster(&self->cursor.str_ite);
    MyStr_get_grapheme_at_column(&self->cursor.str_ite, target_x);

    EditorWindow_make_cursor_visible(self);
    self->selection = self->cursor;
    self->selecting = 1;
}

void EditorWindow_on_hover(Window *win, int x, int y) {
    EditorWindow *self = win;

    if (self->selecting == 0)
        return;

    // Calculate target line
    int target_y = y - win->shift - win->calculated.y;
    target_y = min(target_y, self->n_lines - 1);
    target_y = max(target_y, 0);

    // Calculate target column
    int target_x = x - win->calculated.x;
    target_x = max(target_x, 0);

    // Get the node for the target line
    Node *node = EditorWindow_get_line_number(self, target_y);
    if (node == NULL)
        return;

    // Update cursor position
    self->cursor.y = target_y;
    self->cursor.node = node;

    // Initialize iterator and position at target column
    MyStr_init(&self->cursor.str_ite, node->line);
    MyStr_next_cluster(&self->cursor.str_ite);
    MyStr_get_grapheme_at_column(&self->cursor.str_ite, target_x);

    EditorWindow_make_cursor_visible(self);
}

void EditorWindow_on_mouse_up(Window *win) {
    EditorWindow *self = win;
    self->selecting = 0;
    if (EditorPointer_cmp(&self->cursor, &self->selection) == 0)
        self->selection.y = -1;
}

EditorWindow *latestEditorWindow;
EditorWindow *EditorWindow_new() {
    EditorWindow *self = malloc(sizeof *self);
    memset(self, 0, sizeof *self); // Zero-initialize to prevent garbage values
    Window_init(self, -1, -1, -1, -1, -1, -1);
    self->win.top = 0;
    self->win.bottom = 0;
    self->win.left = 0;
    self->win.right = 0;
    self->win.id = "editor tab";
    self->head = NULL; // Explicitly initialize linked list pointers
    self->tail = NULL;
    self->n_lines = 0;
    self->top_n = 0;
    self->cursor.y = 0;
    self->cursor.x = 0;
    self->selection.y = -1;
    self->selection.x = 0;
    self->highlight_start.y = -1;
    self->highlight_end.y = -1;
    self->language = LANG_NONE;
    self->selecting = 0;

    self->win.draw = EditorWindow_draw;

    self->win.send_key = EditorWindow_send_key;
    self->win.send_sequence = EditorWindow_send_sequence;

    self->win.scroll_wheel_up = EditorWindow_scroll_wheel_up;
    self->win.scroll_wheel_down = EditorWindow_scroll_wheel_down;
    self->win.on_mouse_down = EditorWindow_on_mouse_down;
    self->win.on_hover = EditorWindow_on_hover;
    self->win.on_mouse_up = EditorWindow_on_mouse_up;

    latestEditorWindow = self;

    return self;
}

void status_bar_draw(Window *w, int hasFocus) {
    Geometry geo = w->calculated;
    int y = geo.y;
    StatusWindow *self = w;
    int fg = 232;
    // int bg = 250;
    int bg = 39;
    if (hasFocus == 0)
        bg = 250;
    int line = self->editor->cursor.y + 1;

    int n_lines = self->editor->n_lines;
    int pct = (line * 100) / n_lines;

    /*uint32_t cp = 0;
    Node *node = EditorWindow_get_line_number(self->editor, self->editor->cursor.y);
    LOG_INFO("status_bar_draw %p", node);
    if (node != NULL) {
        int width;
        char *c = char_at(node->line, self->editor->cursor.x, &width);
        if (c != NULL) {
            cp = utf8_decode(&c);
        }
                }*/

    MyStr *str_ite = &self->editor->cursor.str_ite;

    char codepoints_str[256] = "";
    cluster_codepoints_to_string(str_ite, codepoints_str, sizeof(codepoints_str));

    snprintf(self->status_str, sizeof(self->status_str),
             "line:%d/%d (%d%%) column:%d [%s] width:%d "
             "cluster_start:%d cluster_size:%d pos_cluster:%d width_codepoints:%d width_bytes:%d",
             line, n_lines, pct, str_ite->pos_column, codepoints_str, str_ite->width_column,
             str_ite->cluster_start, str_ite->cluster_size, str_ite->pos_cluster,
             str_ite->width_codepoints, str_ite->width_bytes);
    // LOG_INFO("line %s y:%d, geo.x:%d, geo.width:%d", self->status_str, y, geo.x, geo.width);
    Buffer_print(&main_buf, y, geo.x, geo.width, self->status_str, fg, bg);
}

StatusWindow *status_bar_create() {
    StatusWindow *status = malloc(sizeof *status);
    memset(status, 0, sizeof *status); // Zero-initialize to prevent garbage values
    Window_init(status, -1, -1, -1, -1, -1, -1);
    status->win.left = 0;
    status->win.right = 0;
    status->win.height = 1;
    status->win.bottom = 0;
    status->win.draw = status_bar_draw;
    return status;
}

Window *EditorWindow_new_tab(Tabs *self) {
    TabWindow *tab = malloc(sizeof *tab);
    memset(tab, 0, sizeof *tab); // Zero-initialize to prevent garbage values
    tab->tab.icon = "📄";
    Window_init(tab, -1, -1, -1, -1, -1, -1);
    tab->win.left = 0;
    tab->win.right = 0;
    tab->win.top = 0;
    tab->win.bottom = 0;

    EditorWindow *editor = EditorWindow_new();
    editor->tab_window = tab;
    Window *slider = slider_new(editor);
    slider->left = 0;
    slider->right = 0;
    slider->top = 0;
    slider->bottom = 1;
    Slider_show_grip(slider);
    editor->slider = slider;

    editor->win.id = "tab";
    slider->id = editor->win.id;

    Window *searchbox = EditorWindow_searchbox(editor);
    editor->search_box = searchbox;

    tab->win.focused = slider;

    StatusWindow *status = status_bar_create();
    status->editor = editor;

    Window_append(tab, slider);
    Window_append(tab, searchbox);
    Window_append(tab, status);

    return tab;
}

// Editor Frame

EditorFrame *last_frame;

EditorWindow *Editor_get_focused_window(EditorFrame *self) {
    Tab *tab = self->tabs->selected_tab;
    return tab->child->head->head;
}

void Editor_on_selected(EditorFrame *self, void fn()) {
    EditorWindow *editor = Editor_get_focused_window(self);
    fn(editor);
}

Window *Editor_menu_new(EditorFrame *self) { LOG_INFO("Editor_menu_new %p", self); }

Window *Editor_menu_close(EditorFrame *self) { LOG_INFO("Editor_menu_close %p", self); }

Window *Editor_set_language(EditorFrame *self, int lang) {
    EditorWindow *ew = Editor_get_focused_window(self);
    ew->language = lang;
}

int Editor_get_current_tab_language(EditorFrame *self) {
    EditorWindow *ew = Editor_get_focused_window(self);
    return ew->language;
}

void Editor_show_tabs(EditorFrame *self, char *show_tabs_label) {
    show_tabs = 1 - show_tabs;
    if (show_tabs) {
        show_tabs_label[1] = 'x';
    } else {
        show_tabs_label[1] = ' ';
    }
}

void EditorWindow_close(EditorFrame *self) {
    Tab *tab = self->tabs->selected_tab;
    tabs_remove_tab(tab);
}

Window *Editor_menu(EditorFrame *self) {
    Window *menu = Menu_create_horizontal();

    Window *file = Menu_create_vertical(self);
    // Menu_add_element(file, " 📄 New    Ctrl+N", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(file, " 🔄 Reload Ctrl+R",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_reload));
    Menu_add_element(file, " 💾 Save   Ctrl+S",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_save));
    Menu_add_element(file, " ❌ Close", create_lambda(EditorWindow_close, 1, self));
    Menu_add_element(file, "", NULL);
    Menu_add_submenu(menu, " File ", file);

    Window *edit = Menu_create_vertical(self);
    Menu_add_element(edit, " ❌ Delete Backspace",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_backspace));
    Menu_add_element(edit, " 🔪 Cut    Ctrl+X",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_cut));
    Menu_add_element(edit, " 📋 Copy   Ctrl+C",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_copy));
    Menu_add_element(edit, " 📌 Paste  Ctrl+V",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_paste));
    Menu_add_element(edit, " ↩️ Undo   z",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_undo));
    Menu_add_element(edit, " ↪️ Redo   Z",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_redo));
    Menu_add_element(edit, "", NULL);
    Menu_add_submenu(menu, " Edit ", edit);

    Window *view = Menu_create_vertical(self);
    // Menu_add_element(view, " ⤶ Word wrap", create_lambda(Editor_menu_new, 1, self));
    char *show_tabs_label = strdup("   Show Tabs");
    Menu_add_element(view, show_tabs_label,
                     create_lambda(Editor_show_tabs, 2, self, show_tabs_label));
    Menu_add_element(view, "", NULL);
    Menu_add_submenu(menu, " View ", view);

    Window *syntax = Menu_create_vertical(self);
    Menu_add_element(syntax, strdup("   None"),
                     create_lambda(Editor_set_language, 2, self, LANG_NONE));
    Menu_add_element(syntax, strdup("   🔧 C"),
                     create_lambda(Editor_set_language, 2, self, LANG_C));
    Menu_add_element(syntax, strdup("   💠 C++"),
                     create_lambda(Editor_set_language, 2, self, LANG_CPP));
    Menu_add_element(syntax, strdup("   ☕ Java"),
                     create_lambda(Editor_set_language, 2, self, LANG_JAVA));
    Menu_add_element(syntax, strdup("   📜 JavaScript"),
                     create_lambda(Editor_set_language, 2, self, LANG_JS));
    Menu_add_element(syntax, strdup("   📝 TypeScript"),
                     create_lambda(Editor_set_language, 2, self, LANG_TS));
    Menu_add_element(syntax, strdup("   🐍 Python"),
                     create_lambda(Editor_set_language, 2, self, LANG_PY));
    Menu_add_element(syntax, "", NULL);
    Menu_add_submenu(menu, " Sytnax ", syntax);
    syntax->lambda = create_lambda(Editor_get_current_tab_language, 1, self);

    Menu_add_windows(menu, " Window ", self->tabs->win.data, self);

    return menu;
}

Window *Editor_toolbar(EditorFrame *self) {
    Window *toolbar = Menu_create_horizontal();
    Menu_add_element(toolbar, " 💾 Save ",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_save));
    // Menu_add_element(toolbar, " 📄 New ", create_lambda(Editor_menu_new, 1, self));
    // Menu_add_element(toolbar, " 🔄 Undo ", create_lambda(Editor_menu_new, 1, self));
    // Menu_add_element(toolbar, " ❌ Close ", create_lambda(Editor_menu_new, 1, self));
    // Menu_add_element(toolbar, " ❌ Delete ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " 🔪 Cut ",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_cut));
    Menu_add_element(toolbar, " 📋 Copy ",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_copy));
    Menu_add_element(toolbar, " 📌 Paste ",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_paste));
    Menu_add_element(toolbar, " ↩️ Undo ",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_undo));
    Menu_add_element(toolbar, " ↪️ Redo ",
                     create_lambda(Editor_on_selected, 2, self, EditorWindow_redo));

    toolbar->top = 1;

    return toolbar;
}

Window *_Editor_searchbox(EditorFrame *self) {
    EditorWindow *search_box = EditorWindow_new();
    search_box->win.top = 1;
    search_box->win.left = -1;
    search_box->win.right = 1;
    search_box->win.width = 15;
    search_box->win.height = 1;
    return search_box;
}

Window *Editor_new(int left, int right, int top, int bottom, int width, int height) {
    EditorFrame *editor_frame = malloc(sizeof *editor_frame);
    memset(editor_frame, 0, sizeof *editor_frame); // Zero-initialize
    Window *frame = editor_frame;
    Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 0);

    Window *tabs = Tab_new((Window * (*)(Tabs *self)) EditorWindow_new_tab, 0);
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

void Editor_open_file(EditorFrame *editor_frame, char *file_path) {
    if (file_path == NULL) {
        return;
    }
    if (editor_frame == NULL) {
        return;
    }
    if (editor_frame->tabs == NULL) {
        return;
    }
    // if file already been opened, give focus
    LOG_INFO("Editor_open_file: checking if already open %p", editor_frame->tabs->first);
    Tab *tab = editor_frame->tabs->first;
    int tab_count = 0;
    while (tab != NULL) {
        tab_count++;
        if (tab_count > 100) {
            LOG_INFO("Editor_open_file: ERROR infinite loop detected in tab list!");
            break;
        }
        if (tab->child != NULL && tab->child->head != NULL && tab->child->head->head != NULL) {
            EditorWindow *child = tab->child->head->head;
            LOG_INFO("Editor_open_file: tab has child, file_path=%p", child->file_path);
            if (child->file_path != NULL && strcmp(file_path, child->file_path) == 0) {
                LOG_INFO("Editor_open_file: file already open, switching to tab");
                tab_select(tab);
                return;
            }
        }
        LOG_INFO("Editor_open_file: moving to next tab");
        tab = tab->next;
    }

    // otherwise load new file
    Window *slider = tabs_new_tab(editor_frame->tabs);
    if (latestEditorWindow == NULL) {
        LOG_INFO("Editor_open_file: ERROR latestEditorWindow is NULL!");
        return;
    }
    EditorWindow_open_file(latestEditorWindow, file_path);
}

void Editor_last_open_file(char *file_path) {
    if (file_path == NULL) {
        LOG_INFO("Editor_last_open_file: file_path is NULL");
        return;
    }
    if (last_frame == NULL) {
        file_editor_new();
    }
    if (last_frame == NULL) {
        LOG_INFO("Editor_last_open_file: last_frame is still NULL after file_editor_new");
        return;
    }
    Editor_open_file(last_frame, file_path);
    TaskBar_switch_frame(last_frame);
}
