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
#include "text_edit.h"
#include "clipboard.h"
#include "lexer.h"
#include "utils.h"
#include "common.h"
#include "config.h"



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

void update_lexer_state(Node * current, int fast, int language){
    /*
    * Run the lexer on all lines and save the lexer state on each line
    * so that we can run the lexer on a random line as if we processed each line before it
    */
    int lastLexerState = current->lexerState;
    int i = 0;
    while (current != NULL){
        if (i > 0 && fast == 1 && current->lexerState == lastLexerState) return;
        current->lexerState = lastLexerState;
        Lexer lexer;
        Token token;
        token.start = 0;
        lexer_init(&lexer, current->line, language);
        lexer.state = lastLexerState;
        while(lexer_next(&lexer, &token)){}
        lastLexerState = lexer.state;
        current = current->next;
        i++;
    }
}

void EditorWindow_insert(EditorWindow *self, char c){
    Node * node = EditorWindow_get_line_number(self, self->cursor_n);

    // Calculate byte position BEFORE potential realloc
    size_t byte_pos = (uint8_t *)self->cursor_ptr - (uint8_t *)node->line;

    if (node->capacity <= node->length+1){
        double_capacity(node);
        // After realloc, cursor_ptr is invalid! Recalculate it
        self->cursor_ptr = node->line + byte_pos;
    }

    insert_char(node->line, byte_pos, c, node->length, node->capacity);
    node->length++;  // Increment byte length

    int w = 1;
    if (c == '\t') {
        w = tab_width;
    }
    self->cursor_x += w;
    node->width += w;

    // Update cursor_ptr to point after the inserted character
    self->cursor_ptr = node->line + byte_pos + 1;

    //update_lexer_state(self->head);
	EditorWindow_make_cursor_visible(self);
    update_lexer_state(node, 1, self->language);
}

void Node_append(Node *node, char *text)
{
    if (!node || !text || *text == '\0')
        return;

    size_t text_len = strlen(text); // should this be calculate_width?
    int width = calculate_width(text); // should this be calculate_width?

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
    node->width += width;
    node->line[node->length] = '\0';
}

void EditorWindow_delete(EditorWindow *self){
    // delete a character
    if (self->cursor_x == 0){
        if (self->cursor_n != 0){
            Node * node = EditorWindow_get_line_number(self, self->cursor_n);
            Node * next = node->next;
            Node * prev = node->prev;
            self->cursor_x = prev->width;  // Use width (display), not length (bytes)
            self->cursor_ptr = prev->line + prev->length;  // Point to end of prev line
            self->cursor_n--;
            self->n_lines--;
            Node_append(prev, node->line);
            prev->next = next;
            if (next == NULL) self->tail = next;
            else next->prev = prev;
            update_lexer_state(node, 1, self->language);
        }
        return;
    } else {
        Node * node = EditorWindow_get_line_number(self, self->cursor_n);

        // Use cursor_ptr which already points to the correct position
        const uint8_t *char_start = (const uint8_t *)self->cursor_ptr;

        // Move back one character
        const uint8_t *char_ptr = char_start;
        uint32_t cp = utf8_decode_left(&char_ptr, (const uint8_t *)node->line);
        if (char_ptr < (const uint8_t *)node->line) return;  // Already at start

        // Decode again from the found position to get byte length
        const uint8_t *p = char_ptr;
        const uint8_t *start = p;
        cp = utf8_decode(&p);
        int w = cp_width(cp);  // Display width
        int byte_len = p - start;  // Byte length of the UTF-8 character

        // Calculate byte position in the string
        size_t byte_pos = char_ptr - (const uint8_t *)node->line;

        // Delete the UTF-8 character (may be multiple bytes) in a single operation
        memmove(node->line + byte_pos,
                node->line + byte_pos + byte_len,
                node->length - byte_pos - byte_len + 1);  // +1 for null terminator

        // Update both byte length and display width
        node->length -= byte_len;
        node->width -= w;
        self->cursor_x -= w;
        self->cursor_ptr = (char *)char_ptr;  // Update cursor pointer
        update_lexer_state(node, 1, self->language);
    }
    //update_lexer_state(self->head);
}

void EditorWindow_copy(EditorWindow *self){
    if (self->selection_n == -1) return;
    // get range
    int i1 = self->cursor_n;
    //int j1 = self->cursor_x;
	uint8_t * j1 = self->cursor_ptr;
    int i2 = self->selection_n;
    //int j2 = self->selection_x;
	uint8_t * j2 = self->selection_ptr;
    if (self->selection_n < self->cursor_n || 
       (self->selection_n == self->cursor_n && self->selection_x < self->cursor_x)){
        i1 = self->selection_n;
        j1 = self->selection_ptr;
        i2 = self->cursor_n;
        j2 = self->cursor_ptr;
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
                // Copy from j1 to j2 (both on same line)
                memcpy(buffer_cursor, j1, j2-j1);
                buffer_cursor += j2-j1;
            } else { // first item only
                // Copy from j1 to end of line
                size_t len = node->length - (j1 - (uint8_t*)node->line);
                memcpy(buffer_cursor, j1, len);
                buffer_cursor += len;
                *buffer_cursor = '\n';
                buffer_cursor++;
            }
        } else if (i == i2-i1){ // last item
            // Copy from start of line to j2
            size_t len = j2 - (uint8_t*)node->line;
            memcpy(buffer_cursor, node->line, len);
            buffer_cursor += len;
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

    // Check if clipboard is empty
    if (cb == NULL || *cb == '\0') {
        if (cb) free(cb);
        return;
    }

    // Use cursor_ptr to get the byte position for splitting the line
    size_t byte_pos = (uint8_t *)self->cursor_ptr - (uint8_t *)current->line;

    char * tail = strdup(self->cursor_ptr);
    current->line[byte_pos] = '\0';
    current->length = byte_pos;  // byte length
    current->width = self->cursor_x;  // display width

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
            new_node->width = calculate_width(line);
            new_node->capacity = new_node->length+1;
            new_node->lexerState = 0;
            new_node->next = NULL;  // Initialize to NULL to prevent garbage pointer
            current->next = new_node;
            new_node->prev = current;
            current = new_node;
            self->n_lines++;
        }

        if (end == NULL || *line == '\0'){
            break;
        }

        line = end + 1;
        i += 1;
    }

    // Save byte length before appending tail
    size_t pos_before_tail = current->length;

    Node_append(current, tail);
    free(tail);

    // Update cursor position to where we were before appending tail
    self->cursor_x = calculate_width_n(current->line, pos_before_tail);
    self->cursor_ptr = current->line + pos_before_tail;

    current->next = next_line;
    if (next_line != NULL) {
        next_line->prev = current;
    } else {
        self->tail = current;
    }

    self->cursor_n += i;

    free(cb);

    update_lexer_state(current, 1, self->language);
}

void EditorWindow_delete_region(EditorWindow *self){
    if (self->selection_n == -1) return;
    // get range
    int i1 = self->cursor_n;
    int j1_width = self->cursor_x;  // display width
    uint8_t *j1_ptr = (uint8_t *)self->cursor_ptr;  // byte pointer
    int i2 = self->selection_n;
    int j2_width = self->selection_x;  // display width
    uint8_t *j2_ptr = (uint8_t *)self->selection_ptr;  // byte pointer

    if (self->selection_n < self->cursor_n ||
       (self->selection_n == self->cursor_n && self->selection_x < self->cursor_x)){
        i1 = self->selection_n;
        j1_width = self->selection_x;
        j1_ptr = (uint8_t *)self->selection_ptr;
        i2 = self->cursor_n;
        j2_width = self->cursor_x;
        j2_ptr = (uint8_t *)self->cursor_ptr;
    }

    Node * node1 = EditorWindow_get_line_number(self, i1);
    Node * node2 = EditorWindow_get_line_number(self, i2);

    // Use byte pointers instead of display widths
    char * tail = (char *)j2_ptr;
    size_t byte_pos1 = j1_ptr - (uint8_t *)node1->line;

    node1->line[byte_pos1] = 0;
    node1->length = byte_pos1;  // byte length
    node1->width = j1_width;  // display width
    Node_append(node1, tail);

    node1->next = node2->next;
    if (node2->next == NULL){
        self->tail = node1;
    } else {
        node2->next->prev = node1;
    }

    self->cursor_n = i1;
    self->cursor_x = j1_width;
    self->cursor_ptr = node1->line + byte_pos1;

    self->selection_n = -1;
    self->n_lines -= i2-i1;

    update_lexer_state(node1, 1, self->language);
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

    // Use cursor_ptr (byte position) instead of cursor_x (display width)
    new_node->line = strdup(self->cursor_ptr);
    new_node->length = strlen(new_node->line);
    new_node->width = calculate_width(new_node->line);
    new_node->capacity = new_node->length + 1;
    new_node->next = next;
    new_node->prev = node;
    new_node->lexerState = 0;

    if (next == NULL) self->tail = new_node;
    else next->prev = new_node;

    // Truncate current line at cursor position
    size_t byte_pos = (uint8_t *)self->cursor_ptr - (uint8_t *)node->line;
    node->line[byte_pos] = '\0';
    node->length = byte_pos;  // byte length, not display width
    node->width = self->cursor_x;  // display width

    self->n_lines++;
    self->cursor_n++;
    self->cursor_x = 0;
    self->cursor_ptr = new_node->line;  // Point to start of new line
}

void EditorWindow_fix_cursor_x(EditorWindow *self){
    Node * node = EditorWindow_get_line_number(self, self->cursor_n);
    if (node->width < self->cursor_x){
        self->cursor_x = node->width;
    }
    //Node * node = EditorWindow_get_line_number(self, self->cursor_n);
    if (self->cursor_x == node->width){
      self->cursor_ptr = node->line + node->length;
    } else {
      self->cursor_ptr = char_at(node->line, self->cursor_x, &self->cursor_x);
    }
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
    if (text == NULL) {
        LOG_INFO("create_node: text is NULL!");
        text = "";
    }
    Node *new_node = malloc(sizeof(Node));
    if (!new_node)
    {
        perror("malloc failed");
        exit(1);
    }
    //replace_nonprintable(text);
    new_node->line = strdup(text); // copy string
    new_node->length = strlen(text);
    new_node->width = calculate_width(text);
    new_node->capacity = new_node->length + 1;
    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->lexerState = 0;

    return new_node;
}

void append(EditorWindow *self, const char *text)
{
    LOG_INFO("append: self=%p text_len=%zu", self, text ? strlen(text) : 0);
    if (self == NULL) {
        LOG_INFO("append: ERROR self is NULL!");
        return;
    }
    LOG_INFO("append: creating node");
    Node *new_node = create_node(text);
    LOG_INFO("append: node created=%p", new_node);

    if (self->head == NULL)
    {
        LOG_INFO("append: setting head");
        self->head = new_node;
        // return;
    }
    LOG_INFO("append: self->tail=%p", self->tail);
    if (self->tail != NULL) {
        LOG_INFO("append: linking tail->next");
        self->tail->next = new_node;
    }
    LOG_INFO("append: setting new_node->prev");
    new_node->prev = self->tail;
    LOG_INFO("append: updating tail");
    self->tail = new_node;
    LOG_INFO("append: completed");
}

#define MAX_LINE 10240

void load_file(EditorWindow *self, const char *filename)
{
    LOG_INFO("load_file: self=%p filename=%s", self, filename);
    if (self == NULL) {
        LOG_INFO("load_file: ERROR self is NULL!");
        return;
    }
    LOG_INFO("load_file: calling Window_set_id_from_path");
    Window_set_id_from_path(self, "📝", filename);
    LOG_INFO("load_file: opening file");
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("fopen failed");
        return;
    }

    LOG_INFO("load_file: reading file");
    char buffer[MAX_LINE];
    //char expanded[MAX_LINE];

    int line_count = 0;
    while (fgets(buffer, MAX_LINE, file))
    {
        line_count++;
        // Optional: remove newline
        buffer[strcspn(buffer, "\n")] = '\0';

        // Replace tabs with spaces (4 spaces per tab)
        /*int j = 0;
        for (int i = 0; buffer[i] != '\0' && j < MAX_LINE - 4; i++) {
            if (buffer[i] == '\t') {
                // Replace tab with 4 spaces
                expanded[j++] = ' ';
                expanded[j++] = ' ';
                expanded[j++] = ' ';
                expanded[j++] = ' ';
            } else {
                expanded[j++] = buffer[i];
            }
        }
        expanded[j] = '\0';*/

        //LOG_INFO("load_file: line %d, length=%zu", line_count, strlen(expanded));
        append(self, buffer);
        self->n_lines++;
    }
    LOG_INFO("load_file: file read complete, lines=%d", self->n_lines);
    if (self->n_lines == 0){
        append(self, "");
        self->n_lines++;
    }

    fclose(file);

    self->win.virtual_height = self->n_lines;

    self->cursor_ptr = self->head->line;
}

void EditorWindow_run_lexer(EditorWindow *self){
   update_lexer_state(self->head, 0, self->language);
}

void EditorWindow_open_file(EditorWindow *editor_window, char *file_path)
{
    LOG_INFO("EditorWindow_open_file: editor_window=%p file_path=%s", editor_window, file_path);
    if (editor_window == NULL) {
        LOG_INFO("EditorWindow_open_file: ERROR editor_window is NULL!");
        return;
    }
    LOG_INFO("EditorWindow_open_file: slider=%p", editor_window->slider);
    load_file(editor_window, file_path);
    LOG_INFO("EditorWindow_open_file: file loaded");
    if (editor_window->slider != NULL) {
        editor_window->slider->id = file_path;
    }
    editor_window->file_path = strdup(file_path);
    if (ends_with_ignore_case(file_path, ".c")) editor_window->language = LANG_C;
    else if (ends_with_ignore_case(file_path, ".h")) editor_window->language = LANG_C;
    else if (ends_with_ignore_case(file_path, ".cpp")) editor_window->language = LANG_CPP;
    else if (ends_with_ignore_case(file_path, ".hpp")) editor_window->language = LANG_CPP;
    else if (ends_with_ignore_case(file_path, ".java")) editor_window->language = LANG_JAVA;
    else if (ends_with_ignore_case(file_path, ".js")) editor_window->language = LANG_JS;
    else if (ends_with_ignore_case(file_path, ".ts")) editor_window->language = LANG_TS;
    else if (ends_with_ignore_case(file_path, ".py")) editor_window->language = LANG_PY;
    LOG_INFO("EditorWindow_open_file: running lexer");
    EditorWindow_run_lexer(editor_window);
    LOG_INFO("EditorWindow_open_file: completed");
}

void EditorWindow_reload(EditorWindow *editor_window)
{
    editor_window->head = NULL;
    editor_window->tail = NULL;
    editor_window->cursor_n = 0;
    editor_window->cursor_x = 0;
    editor_window->n_lines = 0;
    editor_window->selection_n = -1;
    EditorWindow_open_file(editor_window, editor_window->file_path);
}

void EditorWindow_start_selection(EditorWindow *self){
  self->selection_n = self->cursor_n;
  self->selection_x = self->cursor_x;
  self->selection_ptr = self->cursor_ptr;
}

void _EditorWindow_up(EditorWindow *self){
  self->cursor_n--;
  self->cursor_n = max(self->cursor_n, 0);
  EditorWindow_fix_cursor_x(self);	
  EditorWindow_make_cursor_visible(self);
}

void _EditorWindow_down(EditorWindow *self){
  self->cursor_n++;
  self->cursor_n = min(self->cursor_n, self->n_lines - 1);
  EditorWindow_fix_cursor_x(self);
  EditorWindow_make_cursor_visible(self);
}

void _EditorWindow_right(EditorWindow *self){
  if (* self->cursor_ptr){
	Node * node = EditorWindow_get_line_number(self, self->cursor_n);
	uint32_t cp = utf8_decode(&self->cursor_ptr);
	int w = cp_width(cp);
	self->cursor_x += w;
  } else {
	if (self->cursor_n < self->n_lines-1){
	  self->cursor_n += 1;
	  self->cursor_x = 0;
	  Node * node = EditorWindow_get_line_number(self, self->cursor_n);
	  self->cursor_ptr = node->line;
	}
  }
}

void _EditorWindow_left(EditorWindow *self){
  if (self->cursor_x == 0){
	if (self->cursor_n > 0){
	  self->cursor_n -= 1;
	  Node * node = EditorWindow_get_line_number(self, self->cursor_n);
	  self->cursor_x = node->width;
	  self->cursor_ptr = node->line + node->length;
	}
  } else {	
	Node * node = EditorWindow_get_line_number(self, self->cursor_n);
	uint32_t cp = utf8_decode_left(&self->cursor_ptr, node->line);
	int w = cp_width(cp);
	self->cursor_x -= w;
  }
}

void EditorWindow_up(EditorWindow *self){
  _EditorWindow_up(self);
  if (insert_mode == 1) {
	EditorWindow_start_selection(self);
	//self->selection_n = -1;
  }
}

void EditorWindow_down(EditorWindow *self){
  _EditorWindow_down(self);
  if (insert_mode == 1) {
	EditorWindow_start_selection(self);
	//self->selection_n = -1;
  }
}

void EditorWindow_right(EditorWindow *self){
  _EditorWindow_right(self);
  if (insert_mode == 1) {
	EditorWindow_start_selection(self);
	//self->selection_n = -1;
  }
}

void EditorWindow_left(EditorWindow *self){
  _EditorWindow_left(self);
  if (insert_mode == 1) {
	EditorWindow_start_selection(self);
	//self->selection_n = -1;
  }
}

void EditorWindow_shift_up(EditorWindow *self){
  _EditorWindow_up(self);
}

void EditorWindow_shift_down(EditorWindow *self){
  _EditorWindow_down(self);
}

void EditorWindow_shift_right(EditorWindow *self){
  _EditorWindow_right(self);
}

void EditorWindow_shift_left(EditorWindow *self){
  _EditorWindow_left(self);
}

void EditorWindow_send_sequence(Window *win, const char *seq, int len){
  EditorWindow *self = win;
  if (strlen(seq) == 0){ insert_mode = 0; return; }
  if (strcmp(seq, "[A") == 0){ EditorWindow_up(self); return; }
  if (strcmp(seq, "[B") == 0){ EditorWindow_down(self); return; }
  if (strcmp(seq, "[C") == 0){ EditorWindow_right(self); return; }
  if (strcmp(seq, "[D") == 0){ EditorWindow_left(self); return; }
  if (strcmp(seq, "[1;2A") == 0){ EditorWindow_shift_up(self); return; }
  if (strcmp(seq, "[1;2B") == 0){ EditorWindow_shift_down(self); return; }
  if (strcmp(seq, "[1;2C") == 0){ EditorWindow_shift_right(self); return; }
  if (strcmp(seq, "[1;2D") == 0){ EditorWindow_shift_left(self); return; }
}

void EditorWindow_send_key(Window *win, char c)
{
  EditorWindow *self = win;
  Action action = get_mapping()[c];

  if (action == ACTION_MODE){
	insert_mode = 1 - insert_mode;
	return;
  }
  if (action == ACTION_BACKSPACE){
	EditorWindow_delete(self);
	self->selection_n = -1;
	return;
  }
  if (action == ACTION_ENTER){
	EditorWindow_newline(self);
	return;
  }
  if (action == ACTION_START_OF_LINE){
	self->cursor_x = 0;
	EditorWindow_fix_cursor_x(self);
	return;
  }
  if (action == ACTION_INSERT){
	insert_mode = 1;
	return;
  }
  if (action == ACTION_LEFT){
	EditorWindow_left(self);
	return;
  }
  if (action == ACTION_RIGHT){
	EditorWindow_right(self);
	return;
  }
  if (action == ACTION_END_OF_LINE){
	Node * node = EditorWindow_get_line_number(self, self->cursor_n);
	int mx = node->width;
	self->cursor_x = mx;
	EditorWindow_fix_cursor_x(self);
	return;
  }
  if (action == ACTION_DOWN){
	EditorWindow_down(self);
	return;
  }
  if (action == ACTION_FIRST_LINE){
	self->cursor_n = self->n_lines - 1;
	EditorWindow_fix_cursor_x(self);
	EditorWindow_make_cursor_visible(self);
	return;
  }
  if (action == ACTION_UP){
	EditorWindow_up(self);
	return;
  }
  if (action == ACTION_LAST_LINE){
	self->cursor_n = 0;
	EditorWindow_fix_cursor_x(self);
	EditorWindow_make_cursor_visible(self);
	return;
  }
  if (action == ACTION_PAGE_UP){
	self->cursor_n += win->calculated.height;
	self->cursor_n = min(self->cursor_n, self->n_lines - 1);
	EditorWindow_make_cursor_visible(self);
	return;
  }
  if (action == ACTION_PAGE_DOWN){
	self->cursor_n -= win->calculated.height;
	self->cursor_n = max(self->cursor_n, 0);
	EditorWindow_make_cursor_visible(self);
	return;
  }
  if (action == ACTION_START_SELECTION){
	//self->selection_n = self->cursor_n;
	//self->selection_x = self->cursor_x;
	EditorWindow_start_selection(self);
	return;
  }
  if (action == ACTION_COPY){
	EditorWindow_copy(self);
	self->selection_n = -1;
	return;
  }
  if (action == ACTION_PASTE){
	EditorWindow_paste(self);
	return;
  }
  if (action == ACTION_CUT){
	EditorWindow_cut(self);
	return;
  }
  if (action == ACTION_SAVE){
	EditorWindow_save(self);
	return;
  }
  if (action == ACTION_RELOAD){
	EditorWindow_reload(self);
	return;
  }
  if (insert_mode == 1){
	self->selection_n = -1;
	EditorWindow_insert(self, c);
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
        // background color
        int bg = 255;
        //bg = 236;
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
                    idx2 = current->width;
                }
                // if selection marker below
                if (self->cursor_n < self->selection_n){
                    idx1 = 0;
                    idx2 = self->selection_x;
                }
                int diff = idx2 - idx1;
                //Buffer_print(&main_buf, geo.y + i, geo.x+idx1, diff, str+idx1, 16, 27);
                Buffer_set_bg(&main_buf, geo.y + i, geo.x+idx1, diff, 27);
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
                    idx2 = current->width;
                }
                int diff = idx2 - idx1;
                //Buffer_print(&main_buf, geo.y + i, geo.x+idx1, diff, str+idx1, 16, 27);
                Buffer_set_bg(&main_buf, geo.y + i, geo.x+idx1, diff, 27);
            }
        }
        
        if (-self->win.shift + i == self->cursor_n){ // show cursor
            bg = 248;
            if (insert_mode == 1) bg = 1;
            //Buffer_print(&main_buf, geo.y + i, geo.x+self->cursor_x, 1, str+self->cursor_x, 16, bg);
			//int idx = get_idx_pos(str, self->cursor_x);
            //Buffer_set_bg(&main_buf, geo.y + i, geo.x+idx, 1, bg);
			Buffer_set_bg(&main_buf, geo.y + i, geo.x+self->cursor_x, 1, bg);
        }

        // syntax highlighter

        if (self->language > LANG_NONE){
            Lexer lexer;
            Token token;
            lexer_init(&lexer, str, self->language);
            if (current != NULL)
                lexer.state = current->lexerState;
            while(lexer_next(&lexer, &token)){
                int width = token.end - token.start;
                int maxWidth = geo.width - token.start;
                width = min(width, maxWidth);
                Buffer_set_fg(&main_buf, geo.y + i, geo.x+token.start, width, token.color);
                //LOG_INFO("lexer_next %d %d %d", token.start, token.end, token.color);
            }
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
    memset(self, 0, sizeof *self);  // Zero-initialize to prevent garbage values
    Window_init(self, -1, -1, -1, -1, -1, -1);
    self->win.top = 0;
    self->win.bottom = 0;
    self->win.left = 0;
    self->win.right = 0;
    self->win.id = "editor tab";
    self->head = NULL;  // Explicitly initialize linked list pointers
    self->tail = NULL;
    self->n_lines = 0;
    self->top_n = 0;
    self->cursor_n = 0;
    self->cursor_x = 0;
    //self->insert_mode = 0;
    self->selection_n = -1;
    self->selection_x = 0;
    self->language = LANG_NONE;

    // Window *editor = (Window *) self;
    self->win.draw = EditorWindow_draw;

    self->win.send_key = EditorWindow_send_key;
    self->win.send_sequence = EditorWindow_send_sequence;

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


// Editor Frame

EditorFrame *last_frame;

EditorWindow *Editor_get_focused_window(EditorFrame *self){
    Tab * tab = self->tabs->selected_tab;
    return tab->child->head;
}

void Editor_on_selected(EditorFrame *self, void fn()){
    EditorWindow * editor = Editor_get_focused_window(self);
    fn(editor);
}

Window *Editor_menu_new(EditorFrame *self)
{
    LOG_INFO("Editor_menu_new %p", self);
}

Window *Editor_menu_close(EditorFrame *self)
{
    LOG_INFO("Editor_menu_close %p", self);
}

Window *Editor_set_language(EditorFrame *self, int lang){
    EditorWindow * ew = Editor_get_focused_window(self);
    ew->language = lang;
}


int Editor_get_current_tab_language(EditorFrame *self){
    EditorWindow * ew = Editor_get_focused_window(self);
    return ew->language;
}

Window *Editor_menu(EditorFrame *self)
{
    Window *menu = Menu_create_horizontal();

    Window *file = Menu_create_vertical(self);
    Menu_add_element(file, " 📄 New    Ctrl+N", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(file, " 🔄 Reload Ctrl+R", create_lambda(Editor_on_selected, 2, self, EditorWindow_reload));
    Menu_add_element(file, " ❌ Close  Ctrl+W", create_lambda(Editor_menu_new, 1, self));
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

    Window *syntax = Menu_create_vertical(self);
    Menu_add_element(syntax, strdup("   None"), create_lambda(Editor_set_language, 2, self, LANG_NONE));
    Menu_add_element(syntax, strdup("   🔧 C"), create_lambda(Editor_set_language, 2, self, LANG_C));
    Menu_add_element(syntax, strdup("   💠 C++"), create_lambda(Editor_set_language, 2, self, LANG_CPP));
    Menu_add_element(syntax, strdup("   ☕ Java"), create_lambda(Editor_set_language, 2, self, LANG_JAVA));
    Menu_add_element(syntax, strdup("   📜 JavaScript"), create_lambda(Editor_set_language, 2, self, LANG_JS));
    Menu_add_element(syntax, strdup("   📝 TypeScript"), create_lambda(Editor_set_language, 2, self, LANG_TS));
    Menu_add_element(syntax, strdup("   🐍 Python"), create_lambda(Editor_set_language, 2, self, LANG_PY));
    Menu_add_element(syntax, "", NULL);
    Menu_add_submenu(menu, " Sytnax ", syntax);
    syntax->lambda = create_lambda(Editor_get_current_tab_language, 1, self);

    Menu_add_windows(menu, " Window ", self->tabs->win.data, self);

    return menu;
}

Window *Editor_toolbar(EditorFrame *self)
{
    Window *toolbar = Menu_create_horizontal();
    Menu_add_element(toolbar, " 📄 New ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " 🔄 Undo ", create_lambda(Editor_menu_new, 1, self));
    //Menu_add_element(toolbar, " ❌ Close ", create_lambda(Editor_menu_new, 1, self));
    //Menu_add_element(toolbar, " ❌ Delete ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " 🔪 Cut ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " 📋 Copy ", create_lambda(Editor_menu_new, 1, self));
    Menu_add_element(toolbar, " 📌 Paste ", create_lambda(Editor_menu_new, 1, self));

    toolbar->top = 1;

    return toolbar;
}

Window *Editor_new(int left, int right, int top, int bottom, int width, int height)
{
    EditorFrame *editor_frame = malloc(sizeof *editor_frame);
    memset(editor_frame, 0, sizeof *editor_frame);  // Zero-initialize
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
    LOG_INFO("Editor_open_file: start, file_path=%s", file_path);
    if (file_path == NULL) {
        LOG_INFO("Editor_open_file: file_path is NULL");
        return;
    }
    if (editor_frame == NULL) {
        LOG_INFO("Editor_open_file: editor_frame is NULL");
        return;
    }
    if (editor_frame->tabs == NULL) {
        LOG_INFO("Editor_open_file: editor_frame->tabs is NULL");
        return;
    }
    // if file already been opened, give focus
    LOG_INFO("Editor_open_file: checking if already open");
    Tab * tab = editor_frame->tabs->first;
    int tab_count = 0;
    while(tab != NULL){
        tab_count++;
        LOG_INFO("Editor_open_file: checking tab %d, tab=%p", tab_count, tab);
        if (tab_count > 100) {
            LOG_INFO("Editor_open_file: ERROR infinite loop detected in tab list!");
            break;
        }
        if (tab->child != NULL && tab->child->head != NULL) {
            EditorWindow * child = tab->child->head;
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
    LOG_INFO("Editor_open_file: checked %d tabs, file not already open", tab_count);

    // otherwise load new file
    LOG_INFO("Editor_open_file: creating new tab");
    Window *slider = tabs_new_tab(editor_frame->tabs);
    LOG_INFO("Editor_open_file: tab created, slider=%p latestEditorWindow=%p", slider, latestEditorWindow);
    if (latestEditorWindow == NULL) {
        LOG_INFO("Editor_open_file: ERROR latestEditorWindow is NULL!");
        return;
    }
    LOG_INFO("Editor_open_file: opening file in editor");
    EditorWindow_open_file(latestEditorWindow, file_path);
    LOG_INFO("Editor_open_file: completed");
}

void Editor_last_open_file(char *file_path)
{
    if (file_path == NULL) {
        LOG_INFO("Editor_last_open_file: file_path is NULL");
        return;
    }
    if (last_frame == NULL)
    {
        file_editor_new();
        // return;
    }
    if (last_frame == NULL) {
        LOG_INFO("Editor_last_open_file: last_frame is still NULL after file_editor_new");
        return;
    }
    LOG_INFO("Editor_last_open_file: opening %s", file_path);
    LOG_INFO("Editor_last_open_file: calling Editor_open_file");
    Editor_open_file(last_frame, file_path);
    LOG_INFO("Editor_last_open_file: Editor_open_file completed");
    // Window_bring_to_bottom(last_frame);
    // root->focused = last_frame;
    LOG_INFO("Editor_last_open_file: calling TaskBar_switch_frame");
    TaskBar_switch_frame(last_frame);
    LOG_INFO("Editor_last_open_file: completed");
}