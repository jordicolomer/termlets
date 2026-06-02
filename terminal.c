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
} terminal_data;

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
                append_line(&td->lines, td->incomplete_line);
                td->incomplete_len = 0;
                continue;
            }

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

void Terminal_draw(struct Window *wg, Geometry geo, int hasFocus){
    char expanded[4096];
    terminal_data *td = wg->data2;

    /* calculate available height (reserve 1 line for incomplete line if present) */
    int available_height = geo.height;
    int has_incomplete = (td->incomplete_len > 0);
    if (has_incomplete)
        available_height--;

    /* start from tail and go back available_height - 1 times */
    LineNode *start = td->lines.tail;
    for (int j = 0; j < available_height - 1 && start && start->prev; j++)
        start = start->prev;

    int fg = 15;
    int bg = 240;
    if (hasFocus) bg = 0;

    /* draw lines forward from start */
    int i = 0;
    for (LineNode *n = start; n && i < available_height; n = n->next){
        expand_tabs(n->line, expanded, sizeof(expanded));
        Buffer_print_raw(&main_buf, geo.y + i++, geo.x, geo.width, expanded, fg, bg);
    }

    /* draw incomplete line if any */
    if (has_incomplete) {
        td->incomplete_line[td->incomplete_len] = '\0';
        expand_tabs(td->incomplete_line, expanded, sizeof(expanded));
        Buffer_print_raw(&main_buf, geo.y + i++, geo.x, geo.width, expanded, fg, bg);
    }
    while (i < available_height){
        Buffer_print_raw(&main_buf, geo.y + i++, geo.x, geo.width, "", fg, bg);
    }
}

void send_key(struct Window *wg, char c){
    terminal_data *td = wg->data2;
    write(td->master, &c, 1);
    //LOG_INFO("send_key %c", c);

    //if (c == '\n')
    Terminal_update(td);
}

Window *Terminal_new(int left, int right, int top, int bottom, int width, int height){
    Window *frame = malloc(sizeof *frame);
    Window *w = Frame_init(frame, left, right, top, bottom, width, height);
    w->draw = Terminal_draw;

    terminal_data *td = malloc(sizeof *td);
    w->data2 = td;
    frame->data2 = td;
    frame->send_key = send_key;

    pid_t pid = forkpty(&td->master, NULL, NULL, NULL);

    if (pid == 0) {
        execl("/bin/sh", "sh", NULL);
        exit(1);
    }

    /*write(td->master, "ll", 2);
    char bs = 0x7f;
    write(td->master, &bs, 1);
    write(td->master, "s", 1);
    write(td->master, "\n", 1);*/

    //td->lines = {0};
    td->lines.head = NULL;
    td->lines.tail = NULL;
    td->incomplete_len = 0;

    Terminal_update(td);
    Terminal_update(td);

    return frame;
}
