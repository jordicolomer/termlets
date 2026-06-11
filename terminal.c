#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <sys/select.h>
#include "window.h"
#include "frame.h"
#include "buffer.h"
#include "logger.h"
#include "slider.h"

typedef struct LineNode {
    char *line;
    struct LineNode *prev;
    struct LineNode *next;
} LineNode;

typedef struct {
    LineNode *head;
    LineNode *tail;
    int size;
} LineList;

void append_line(LineList *list, const char *text)
{
    LineNode *node = malloc(sizeof(*node));
    node->line = strdup(text);
    node->prev = list->tail;
    node->next = NULL;

    if (list->tail)
        list->tail->next = node;
    else
        list->head = node;

    list->tail = node;

    list->size += 1;
}

void free_lines(LineList *list)
{
    LineNode *n = list->head;

    while (n) {
        LineNode *next = n->next;
        free(n->line);
        free(n);
        n = next;
    }

    list->head = list->tail = NULL;
}

typedef struct terminal_data{
    int master;
    LineList lines;
    char incomplete_line[8192];
    size_t incomplete_len;
    Window *terminal;
    //Window *slider;
} terminal_data;

#include <ctype.h>

void strip_ansi(char *s)
{
    char *src = s;
    char *dst = s;

    while (*src) {
        if ((unsigned char)src[0] == 0x1B) {
            src++;

            /* CSI: ESC [ ... final-byte */
            if (*src == '[') {
                src++;

                while (*src) {
                    unsigned char c = (unsigned char)*src++;

                    /* ANSI CSI final byte range */
                    if (c >= 0x40 && c <= 0x7E)
                        break;
                }
                continue;
            }

            /* Other ESC sequences: skip one following byte */
            if (*src)
                src++;

            continue;
        }

        *dst++ = *src++;
    }

    *dst = '\0';
}

void Terminal_update(terminal_data *td){
    char buf[4096];

    /* keep reading until no more data is available */
    while (1)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(td->master, &fds);

        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 10000  /* 10ms timeout */
        };

        int ret = select(td->master + 1, &fds, NULL, NULL, &tv);

        if (ret <= 0)
            break;  /* no more data or error */

        int n = read(td->master, buf, sizeof(buf));

        if (n <= 0)
            break;

        for (int j = 0; j < n; j++) {
            char c = buf[j];

            /* ignore CR */
            if (c == '\r')
                continue;

            /* line complete */
            if (c == '\n') {
                td->incomplete_line[td->incomplete_len] = '\0';
                //strip_ansi(td->incomplete_line);
                append_line(&td->lines, td->incomplete_line);
                td->incomplete_len = 0;
                continue;
            }

            /* handle backspace */
            /*if (c == '\b' || c == 127) {
                if (td->incomplete_len > 0)
                    td->incomplete_len--;
                continue;
            }*/

            if (td->incomplete_len < sizeof(td->incomplete_line) - 1)
                td->incomplete_line[td->incomplete_len++] = c;
        }
    }

    /*
    printf("=== STORED LINES ===\n");

    for (LineNode *n = td->lines.head; n; n = n->next)
        printf("[%s]\n", n->line);

    printf("=== END ===\n");

    free_lines(&td->lines);
    */
}
#define TAB_WIDTH 8

static void expand_tabs(const char *src, char *dst, size_t dst_size)
{
    size_t col = 0;
    size_t out = 0;

    while (*src && out < dst_size - 1) {
        if (*src == '\t') {
            int spaces = TAB_WIDTH - (col % TAB_WIDTH);

            while (spaces-- && out < dst_size - 1) {
                dst[out++] = ' ';
                col++;
            }
        } else {
            dst[out++] = *src;
            col++;
        }

        src++;
    }

    dst[out] = '\0';
}

int Terminal_get_virtual_height(struct Window *wg){
    terminal_data *td = wg->data2;
    int size = td->lines.size;
    int has_incomplete = (td->incomplete_len > 0);
    if (has_incomplete)
        size++;
    return size;
}

void Terminal_draw(struct Window *wg, int hasFocus){
    wg->virtual_height = Terminal_get_virtual_height(wg);

    Geometry geo = wg->calculated;
    //LOG_INFO("Terminal_draw x:%d y:%d", geo.x, geo.y);
    char expanded[4096];
    terminal_data *td = wg->data2;

    /* calculate available height (reserve 1 line for incomplete line if present) */
    int available_height = geo.height;
    int has_incomplete = (td->incomplete_len > 0);
    if (has_incomplete)
        available_height--;

    /* start from tail and go back available_height - 1 times */
    /*LineNode *start = td->lines.tail;
    for (int j = 0; j < available_height - 1 && start && start->prev; j++)
        start = start->prev;*/
    //LOG_INFO("Terminal_draw %d", wg->shift);
    LineNode *start = td->lines.head;
    for (int j = 0; j < -wg->shift && start && start->next; j++)
        start = start->next;

    int fg = 250;
    int bg = 240;
    if (hasFocus){
        fg = 15;
        bg = 0;
    }

    /* draw lines forward from start */
    int i = 0;
    LineNode *n = start;
    for (; n && i < available_height; n = n->next){
        expand_tabs(n->line, expanded, sizeof(expanded));
        strip_ansi(expanded);
        Buffer_print_raw(&main_buf, geo.y + i++, geo.x, geo.width, expanded, fg, bg);
    }

    /* draw incomplete line if any */
    if (has_incomplete && n == NULL) {
        td->incomplete_line[td->incomplete_len] = '\0';
        expand_tabs(td->incomplete_line, expanded, sizeof(expanded));
        strip_ansi(expanded);
        Buffer_print_raw(&main_buf, geo.y + i++, geo.x, geo.width, expanded, fg, bg);
    } else if (n != NULL){
        expand_tabs(n->line, expanded, sizeof(expanded));
        strip_ansi(expanded);
        Buffer_print_raw(&main_buf, geo.y + i++, geo.x, geo.width, expanded, fg, bg);
    }
    while (i <= available_height){
        Buffer_print_raw(&main_buf, geo.y + i++, geo.x, geo.width, "", fg, bg);
    }
}

void send_key(struct Window *wg, char c){
    terminal_data *td = wg->data2;
    write(td->master, &c, 1);
    //LOG_INFO("send_key %c", c);

    //if (c == '\n')
    Terminal_update(td);
    Window *terminal = td->terminal;
    terminal->shift = -(Terminal_get_virtual_height(terminal) - terminal->calculated.height);
    if (terminal->shift > 0) terminal->shift = 0;

    //Window * slider = td->slider;
    //repaint();
    //Slider_update_top(slider);
}

Window *Terminal_window(Window *frame){
    Window *terminal = malloc(sizeof *terminal);
    Window_init(terminal, 0, 0, 0, 0, -1, -1);
    terminal->id = "terminal window";

    terminal->draw = Terminal_draw;

    terminal_data *td = malloc(sizeof *td);
    terminal->data2 = td;
    frame->data2 = td;
    frame->send_key = send_key;

    pid_t pid = forkpty(&td->master, NULL, NULL, NULL);

    if (pid == 0) {
        execl("/bin/sh", "sh", NULL);
        exit(1);
    }

    td->lines.head = NULL;
    td->lines.tail = NULL;
    td->incomplete_len = 0;
    td->terminal = terminal;

    Terminal_update(td);
    Terminal_update(td);

    return terminal;
}

Window *Terminal_new(int left, int right, int top, int bottom, int width, int height){
    Window *frame = malloc(sizeof *frame);
    Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 1);

    // toolbar
    int j = 0;
    int x_offset = 0;
    int widget_width = 12;
    Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " 📄 New File", 232, 254);
    x_offset += widget_width;
    widget_width = 12;
    Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "📁 New Dir", 232, 254);
    x_offset += widget_width;
    widget_width = 8;
    Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "📋 Copy", 232, 254);
    x_offset += widget_width;
    widget_width = 8;
    Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "🔪 Cut", 232, 254);
    x_offset += widget_width;
    widget_width = 10;
    Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "📌 Paste", 232, 254);
    x_offset += widget_width;
    widget_width = 10;
    Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "🔤 Rename", 232, 254);
    x_offset += widget_width;
    widget_width = 10;
    Window_add_widget(w, x_offset, 0, j, -1, -1, 1, "❌ Delete", 232, 254);
    x_offset += widget_width;

    Window * terminal = Terminal_window(frame);

    Window * slider = slider_new(terminal);
    slider->left = 0;
    slider->right = 0;
    slider->top = 1;
    slider->bottom = 0;
    Window_append(w, slider);
    terminal_data *td = terminal->data2;
    //td->slider = slider; 

    return frame;
}
