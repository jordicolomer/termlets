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
} terminal_data;

void Terminal_update(terminal_data *td){
    char buf[4096];
    char current_line[8192];
    size_t current_len = 0;

    for (int i = 0; i < 20; i++) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(td->master, &fds);

        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 150000
        };

        if (select(td->master + 1, &fds, NULL, NULL, &tv) > 0) {

            int n = read(td->master, buf, sizeof(buf));

            if (n <= 0)
                continue;

            for (int j = 0; j < n; j++) {
                char c = buf[j];

                /* ignore CR */
                if (c == '\r')
                    continue;

                /* line complete */
                if (c == '\n') {
                    current_line[current_len] = '\0';
                    append_line(&td->lines, current_line);
                    current_len = 0;
                    continue;
                }

                if (current_len < sizeof(current_line) - 1)
                    current_line[current_len++] = c;
            }
        }
    }

    /* flush final partial line */
    if (current_len > 0) {
        current_line[current_len] = '\0';
        append_line(&td->lines, current_line);
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
    int i = 0;
    for (LineNode *n = td->lines.head; n; n = n->next){
        expand_tabs(n->line, expanded, sizeof(expanded));
        Buffer_print_raw(&main_buf, geo.y + i++, geo.x, geo.width, expanded, 15, 0);
    }
}

void send_key(struct Window *wg, char c){
    terminal_data *td = wg->data2;
    write(td->master, &c, 1);
    LOG_INFO("send_key %c", c);
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

    Terminal_update(td);

    return frame;
}
