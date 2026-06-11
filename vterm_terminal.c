#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <vterm.h>
#include "window.h"
#include "frame.h"
#include "buffer.h"
#include "logger.h"
#include "slider.h"

typedef struct ScrollbackLine {
    VTermScreenCell *cells;
    int cols;
    struct ScrollbackLine *prev;
    struct ScrollbackLine *next;
} ScrollbackLine;

typedef struct ScrollbackList {
    ScrollbackLine *head;
    ScrollbackLine *tail;
    int count;
    int max_size;
} ScrollbackList;

typedef struct vterm_terminal_data {
    int master;
    VTerm *vt;
    VTermScreen *vts;
    Window *terminal;
    int rows;
    int cols;
    ScrollbackList scrollback;
    VTermScreenCallbacks callbacks;
} vterm_terminal_data;

void scrollback_add_line(ScrollbackList *sb, int cols, const VTermScreenCell *cells)
{
    /* create new line node */
    ScrollbackLine *line = malloc(sizeof(ScrollbackLine));
    line->cols = cols;
    line->cells = malloc(cols * sizeof(VTermScreenCell));
    memcpy(line->cells, cells, cols * sizeof(VTermScreenCell));
    line->next = NULL;
    line->prev = sb->tail;

    /* add to tail of list */
    if (sb->tail) {
        sb->tail->next = line;
    } else {
        sb->head = line;
    }
    sb->tail = line;
    sb->count++;

    /* remove oldest line if we exceed max size */
    while (sb->count > sb->max_size && sb->head) {
        ScrollbackLine *old = sb->head;
        sb->head = old->next;
        if (sb->head) {
            sb->head->prev = NULL;
        } else {
            sb->tail = NULL;
        }
        free(old->cells);
        free(old);
        sb->count--;
    }
}

void VTermTerminal_update(vterm_terminal_data *vtd)
{
    char buf[4096];

    /* keep reading until no more data is available */
    while (1)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(vtd->master, &fds);

        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 10000  /* 10ms timeout */
        };

        int ret = select(vtd->master + 1, &fds, NULL, NULL, &tv);

        if (ret <= 0)
            break;  /* no more data or error */

        int n = read(vtd->master, buf, sizeof(buf));

        if (n <= 0)
            break;

        /* feed data to libvterm */
        vterm_input_write(vtd->vt, buf, n);
    }
}

int VTermTerminal_get_virtual_height(struct Window *wg)
{
    vterm_terminal_data *vtd = wg->data2;

    /* total virtual height = scrollback + visible screen */
    return vtd->scrollback.count + vtd->rows;
}

void VTermTerminal_draw(struct Window *wg, int hasFocus)
{
    //LOG_INFO("VTermTerminal_draw");
    vterm_terminal_data *vtd = wg->data2;
    Geometry geo = wg->calculated;

    /* resize vterm if window size changed */
    if (vtd->rows != geo.height || vtd->cols != geo.width) {
        //LOG_INFO("resize");
        vtd->rows = geo.height;
        vtd->cols = geo.width;
        vterm_set_size(vtd->vt, vtd->rows, vtd->cols);

        // also update PTY size
        struct winsize ws = {
            .ws_row = vtd->rows,
            .ws_col = vtd->cols,
            .ws_xpixel = 0,
            .ws_ypixel = 0
        };
        ioctl(vtd->master, TIOCSWINSZ, &ws);
    }

    wg->virtual_height = VTermTerminal_get_virtual_height(wg);

    int fg = 250;
    int bg = 240;
    if (hasFocus) {
        fg = 15;
        bg = 0;
    }

    /* render each row from scrollback and vterm screen */
    VTermPos pos;
    VTermScreenCell cell;
    char line_buf[4096];

    Window *terminal = vtd->terminal;
    int virtual_height = VTermTerminal_get_virtual_height(terminal);

    /* calculate which virtual lines are visible */
    int first_visible_line = -terminal->shift;
    int last_visible_line = first_visible_line + geo.height - 1;
    //LOG_INFO("VTermTerminal_draw %d", terminal->shift);

    /* render visible rows */
    for (int viewport_row = 0; viewport_row < geo.height; viewport_row++) {
        //LOG_INFO("for %d", viewport_row);
        int virtual_line = first_visible_line + viewport_row;

        /* skip if outside virtual content bounds */
        /*if (virtual_line < 0 || virtual_line >= virtual_height) {
            LOG_INFO("continue");
            continue;
        }*/

        int buf_idx = 0;

        /* determine if this line is in scrollback or current screen */
        if (virtual_line < vtd->scrollback.count) {
            //LOG_INFO("scrollback line");
            /* this is a scrollback line - find it in the linked list */
            ScrollbackLine *line = vtd->scrollback.head;
            for (int i = 0; i < virtual_line && line != NULL; i++) {
                line = line->next;
            }

            if (line) {
                /* render scrollback line */
                for (int col = 0; col < line->cols && col < geo.width; col++) {
                    VTermScreenCell *cell_ptr = &line->cells[col];

                    if (cell_ptr->chars[0] == 0) {
                        line_buf[buf_idx++] = ' ';
                    } else {
                        uint32_t c = cell_ptr->chars[0];
                        if (c < 128) {
                            line_buf[buf_idx++] = (char)c;
                        } else {
                            /* handle multi-byte UTF-8 */
                            if (c < 0x80) {
                                line_buf[buf_idx++] = c;
                            } else if (c < 0x800) {
                                line_buf[buf_idx++] = 0xC0 | (c >> 6);
                                line_buf[buf_idx++] = 0x80 | (c & 0x3F);
                            } else if (c < 0x10000) {
                                line_buf[buf_idx++] = 0xE0 | (c >> 12);
                                line_buf[buf_idx++] = 0x80 | ((c >> 6) & 0x3F);
                                line_buf[buf_idx++] = 0x80 | (c & 0x3F);
                            } else {
                                line_buf[buf_idx++] = 0xF0 | (c >> 18);
                                line_buf[buf_idx++] = 0x80 | ((c >> 12) & 0x3F);
                                line_buf[buf_idx++] = 0x80 | ((c >> 6) & 0x3F);
                                line_buf[buf_idx++] = 0x80 | (c & 0x3F);
                            }
                        }
                    }
                }
                /* pad remaining columns with spaces */
                for (int col = line->cols; col < geo.width; col++) {
                    line_buf[buf_idx++] = ' ';
                }
            }
        } else {
            /* this is a current screen line */
            int screen_row = virtual_line - vtd->scrollback.count;
            //LOG_INFO("current screen %d %d", screen_row, virtual_line);

            if (screen_row >= 0 && screen_row < vtd->rows) {
                for (int col = 0; col < vtd->cols && col < geo.width; col++) {
                    pos.row = screen_row;
                    pos.col = col;

                    vterm_screen_get_cell(vtd->vts, pos, &cell);

                    /* extract character (handling UTF-8) */
                    if (cell.chars[0] == 0) {
                        line_buf[buf_idx++] = ' ';
                    } else {
                        uint32_t c = cell.chars[0];
                        if (c < 128) {
                            line_buf[buf_idx++] = (char)c;
                        } else {
                            /* handle multi-byte UTF-8 */
                            if (c < 0x80) {
                                line_buf[buf_idx++] = c;
                            } else if (c < 0x800) {
                                line_buf[buf_idx++] = 0xC0 | (c >> 6);
                                line_buf[buf_idx++] = 0x80 | (c & 0x3F);
                            } else if (c < 0x10000) {
                                line_buf[buf_idx++] = 0xE0 | (c >> 12);
                                line_buf[buf_idx++] = 0x80 | ((c >> 6) & 0x3F);
                                line_buf[buf_idx++] = 0x80 | (c & 0x3F);
                            } else {
                                line_buf[buf_idx++] = 0xF0 | (c >> 18);
                                line_buf[buf_idx++] = 0x80 | ((c >> 12) & 0x3F);
                                line_buf[buf_idx++] = 0x80 | ((c >> 6) & 0x3F);
                                line_buf[buf_idx++] = 0x80 | (c & 0x3F);
                            }
                        }
                    }
                }
            }
        }

        line_buf[buf_idx] = '\0';
        int y = geo.y + viewport_row;
        //LOG_INFO("Buffer_print_raw %s", line_buf);
        Buffer_print_raw(&main_buf, y, geo.x, geo.width, line_buf, fg, bg);
    }
}

void vterm_send_key(struct Window *wg, char c)
{
    vterm_terminal_data *vtd = wg->data2;

    /* handle escape sequences for arrow keys and other special keys */
    if (c == 27) {  /* ESC - this might be the start of an escape sequence */
        /* For now, just pass through the ESC */
        write(vtd->master, &c, 1);
    } else {
        write(vtd->master, &c, 1);
    }

    VTermTerminal_update(vtd);

    Window *terminal = vtd->terminal;
    terminal->shift = -(VTermTerminal_get_virtual_height(terminal) - terminal->calculated.height);
    if (terminal->shift > 0)
        terminal->shift = 0;
}

void vterm_send_sequence(struct Window *wg, const char *seq, int len)
{
    vterm_terminal_data *vtd = wg->data2;
    write(vtd->master, seq, len);

    VTermTerminal_update(vtd);

    Window *terminal = vtd->terminal;
    terminal->shift = -(VTermTerminal_get_virtual_height(terminal) - terminal->calculated.height);
    if (terminal->shift > 0)
        terminal->shift = 0;
}

static int cb_sb_pushline(int cols, const VTermScreenCell *cells, void *user)
{
    vterm_terminal_data *vtd = user;

    scrollback_add_line(&vtd->scrollback, cols, cells);

    /*LOG_INFO("[sb_pushline]\n");
    LOG_INFO("  cols=%d\n", cols);

    for (int i = 0; i < cols; i++) {
        LOG_INFO("    cell[%d]=%c\n", i, cells[i].chars[0]);
    }*/

    return 1;
}

Window *VTermTerminal_window(Window *frame, int initial_rows, int initial_cols)
{
    Window *terminal = malloc(sizeof *terminal);
    Window_init(terminal, 0, 0, 0, 0, -1, -1);
    terminal->id = "vterm terminal window";

    terminal->draw = VTermTerminal_draw;

    vterm_terminal_data *vtd = malloc(sizeof *vtd);
    terminal->data2 = vtd;
    frame->data2 = vtd;
    frame->send_key = vterm_send_key;
    frame->send_sequence = vterm_send_sequence;

    /* initialize libvterm */
    vtd->rows = initial_rows;
    vtd->cols = initial_cols;

    /* initialize scrollback list */
    vtd->scrollback.head = NULL;
    vtd->scrollback.tail = NULL;
    vtd->scrollback.count = 0;
    vtd->scrollback.max_size = 1000;

    vtd->vt = vterm_new(vtd->rows, vtd->cols);
    vtd->vts = vterm_obtain_screen(vtd->vt);

    vtd->callbacks.sb_pushline = cb_sb_pushline;
    vtd->callbacks.sb_popline = NULL;

    vterm_screen_set_callbacks(vtd->vts, &vtd->callbacks, vtd);

    vterm_screen_reset(vtd->vts, 1);
    vterm_screen_enable_altscreen(vtd->vts, 1);

    /* set UTF-8 mode */
    vterm_set_utf8(vtd->vt, 1);

    /* fork pty */
    pid_t pid = forkpty(&vtd->master, NULL, NULL, NULL);

    if (pid == 0) {
        /* child process */
        execl("/bin/sh", "sh", NULL);
        exit(1);
    }

    vtd->terminal = terminal;

    /* initial update */
    VTermTerminal_update(vtd);
    VTermTerminal_update(vtd);

    return terminal;
}

Window *VTermTerminal_new(int left, int right, int top, int bottom, int width, int height)
{
    Window *frame = malloc(sizeof *frame);
    Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 1);

    /* toolbar */
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

    /* create terminal with initial size (will be resized on first draw) */
    Window *terminal = VTermTerminal_window(frame, 24, 80);

    /* create slider */
    Window *slider = slider_new(terminal);
    slider->left = 0;
    slider->right = 0;
    slider->top = 1;
    slider->bottom = 0;
    Window_append(w, slider);

    return frame;
}
