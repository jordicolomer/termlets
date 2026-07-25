#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <vterm.h>
#include <libproc.h>

#include "vterm_terminal.h"
#include "window.h"
#include "frame.h"
#include "buffer.h"
#include "logger.h"
#include "slider.h"
#include "tabs.h"
#include "menu.h"

/* Global state for PTY monitoring thread */
static pthread_t pty_monitor_thread;
static pthread_mutex_t terminals_mutex = PTHREAD_MUTEX_INITIALIZER;
static int need_repaint = 0;
static int thread_running = 0;

/* List of all terminal instances */
#define MAX_TERMINALS 32
static struct vterm_terminal_data *active_terminals[MAX_TERMINALS];
static int terminal_count = 0;

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

void VTermTerminal_scroll_wheel_down(struct Window *w){
  TerminalWindow * self = w;
  Slider_scroll_down(self->slider);
}

void VTermTerminal_scroll_wheel_up(struct Window *w){
  TerminalWindow * self = w;
  Slider_scroll_up(self->slider);
}

char* get_shell_cwd_mac_native(pid_t pid)
{
    struct proc_vnodepathinfo vpi;

    int ret = proc_pidinfo(pid, PROC_PIDVNODEPATHINFO, 0, &vpi, sizeof(vpi));
    if (ret <= 0) {
        return NULL;
    }

    // pvi_cdir is the current working directory
    if (vpi.pvi_cdir.vip_path[0] != '\0') {
        return strdup(vpi.pvi_cdir.vip_path);
    }

    return NULL;
}

void update_tab_label(TerminalWindow * terminal){
    char * cwd = get_shell_cwd_mac_native(terminal->pid);
    Window_set_id_from_path(terminal->slider, "💻", cwd);
}

/* Forward declarations */
int VTermTerminal_get_virtual_height(struct Window *wg);

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

        //update_tab_label(vtd->terminal);
    }
}

/* Register a terminal for monitoring */
void register_terminal(vterm_terminal_data *vtd)
{
    pthread_mutex_lock(&terminals_mutex);
    if (terminal_count < MAX_TERMINALS) {
        active_terminals[terminal_count++] = vtd;
    }
    pthread_mutex_unlock(&terminals_mutex);
}

/* Unregister a terminal from monitoring */
void unregister_terminal(vterm_terminal_data *vtd)
{
    pthread_mutex_lock(&terminals_mutex);
    for (int i = 0; i < terminal_count; i++) {
        if (active_terminals[i] == vtd) {
            /* shift remaining terminals */
            for (int j = i; j < terminal_count - 1; j++) {
                active_terminals[j] = active_terminals[j + 1];
            }
            terminal_count--;
            break;
        }
    }
    pthread_mutex_unlock(&terminals_mutex);
}

/* Background thread that monitors all PTYs */
void *pty_monitor_thread_func(void *arg)
{
    char buf[4096];

    while (thread_running) {
        fd_set fds;
        FD_ZERO(&fds);
        int max_fd = -1;

        /* Build fd_set with all active terminal master FDs */
        pthread_mutex_lock(&terminals_mutex);
        for (int i = 0; i < terminal_count; i++) {
            int fd = active_terminals[i]->master;
            FD_SET(fd, &fds);
            if (fd > max_fd) max_fd = fd;
        }
        pthread_mutex_unlock(&terminals_mutex);

        if (max_fd < 0) {
            /* No terminals, sleep briefly */
            usleep(50000);  /* 50ms */
            continue;
        }

        /* Wait for data with timeout */
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 50000  /* 50ms timeout */
        };

        int ret = select(max_fd + 1, &fds, NULL, NULL, &tv);

        if (ret > 0) {
            /* Data available on one or more terminals */
            pthread_mutex_lock(&terminals_mutex);
            for (int i = 0; i < terminal_count; i++) {
                vterm_terminal_data *vtd = active_terminals[i];

                if (FD_ISSET(vtd->master, &fds)) {
                    /* Read and process data from this terminal */
                    int n = read(vtd->master, buf, sizeof(buf));
                    if (n > 0) {
                        vterm_input_write(vtd->vt, buf, n);

                        /* Update scroll position to follow output */
                        Window *terminal = vtd->terminal;
                        terminal->shift = -(VTermTerminal_get_virtual_height(terminal) - terminal->calculated.height);
                        if (terminal->shift > 0)
                            terminal->shift = 0;

                        /* Signal that repaint is needed */
                        need_repaint = 1;
                    }
                }
            }
            pthread_mutex_unlock(&terminals_mutex);
        }
    }

    return NULL;
}

/* Start the PTY monitoring thread */
void start_pty_monitor_thread()
{
    if (!thread_running) {
        thread_running = 1;
        pthread_create(&pty_monitor_thread, NULL, pty_monitor_thread_func, NULL);
    }
}

/* Stop the PTY monitoring thread */
void stop_pty_monitor_thread()
{
    if (thread_running) {
        thread_running = 0;
        pthread_join(pty_monitor_thread, NULL);
    }
}

/* Check if repaint is needed and clear the flag */
int check_and_clear_repaint_flag()
{
    int result;
    pthread_mutex_lock(&terminals_mutex);
    result = need_repaint;
    need_repaint = 0;
    pthread_mutex_unlock(&terminals_mutex);
    return result;
}

int VTermTerminal_get_virtual_height(struct Window *wg)
{
    vterm_terminal_data *vtd = wg->data2;

    /* total virtual height = scrollback + visible screen */
    return vtd->scrollback.count + vtd->rows;
}

/* convert VTermColor to 256-color palette index */
static int vterm_color_to_256(VTermColor color)
{
    /* if it's an indexed color, use it directly */
    if (VTERM_COLOR_IS_INDEXED(&color)) {
        return color.indexed.idx;
    }

    /* if it's RGB, convert to nearest 256-color */
    if (VTERM_COLOR_IS_RGB(&color)) {
        int r = color.rgb.red;
        int g = color.rgb.green;
        int b = color.rgb.blue;

        /* check for grayscale (232-255) */
        if (r == g && g == b) {
            if (r < 8) return 16;  /* black */
            if (r > 247) return 231;  /* white */
            return 232 + (r - 8) / 10;
        }

        /* convert to 6x6x6 color cube (16-231) */
        int ir = (r * 6) / 256;
        int ig = (g * 6) / 256;
        int ib = (b * 6) / 256;
        return 16 + 36 * ir + 6 * ig + ib;
    }

    /* default colors */
    if (VTERM_COLOR_IS_DEFAULT_FG(&color)) {
        return 7;  /* default foreground */
    }
    if (VTERM_COLOR_IS_DEFAULT_BG(&color)) {
        return 0;  /* default background */
    }

    return 7;  /* fallback */
}

/* Helper function to encode a unicode codepoint to UTF-8 */
static int encode_utf8(uint32_t c, char *buf)
{
    if (c == 0) {
        buf[0] = ' ';
        return 1;
    } else if (c == (uint32_t)-1) {
        /* skip continuation cell for wide characters */
        return 0;
    } else if (c < 0x80) {
        buf[0] = (char)c;
        return 1;
    } else if (c < 0x800) {
        buf[0] = 0xC0 | (c >> 6);
        buf[1] = 0x80 | (c & 0x3F);
        return 2;
    } else if (c < 0x10000) {
        buf[0] = 0xE0 | (c >> 12);
        buf[1] = 0x80 | ((c >> 6) & 0x3F);
        buf[2] = 0x80 | (c & 0x3F);
        return 3;
    } else {
        buf[0] = 0xF0 | (c >> 18);
        buf[1] = 0x80 | ((c >> 12) & 0x3F);
        buf[2] = 0x80 | ((c >> 6) & 0x3F);
        buf[3] = 0x80 | (c & 0x3F);
        return 4;
    }
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

    int default_fg = 250;
    int default_bg = 240;
    if (hasFocus) {
        default_fg = 7;
        default_bg = 0;
    }

    /* render each row from scrollback and vterm screen */
    VTermPos pos;
    VTermScreenCell cell;

    Window *terminal = vtd->terminal;
    int virtual_height = VTermTerminal_get_virtual_height(terminal);

    /* calculate which virtual lines are visible */
    int first_visible_line = -terminal->shift;
    int last_visible_line = first_visible_line + geo.height - 1;
    //LOG_INFO("VTermTerminal_draw %d", terminal->shift);

    ScrollbackLine *line = vtd->scrollback.head;
    for (int i = 0; i < first_visible_line && line != NULL; i++) {
        line = line->next;
    }

    /* render visible rows */
    for (int viewport_row = 0; viewport_row < geo.height; viewport_row++) {
        //LOG_INFO("for %d", viewport_row);
        int virtual_line = first_visible_line + viewport_row;

        int y = geo.y + viewport_row;

        /* determine if this line is in scrollback or current screen */
        if (virtual_line < vtd->scrollback.count) {
            //LOG_INFO("scrollback line");
            /* this is a scrollback line - find it in the linked list */

            if (line) {
                /* render scrollback line with color batching */
                int col = 0;
                while (col < geo.width) {
                    char line_buf[4096];
                    int buf_idx = 0;

                    /* get colors from first cell in batch */
                    int fg = default_fg;
                    int bg = default_bg;
                    if (col < line->cols) {
                        VTermScreenCell *first_cell = &line->cells[col];
                        fg = vterm_color_to_256(first_cell->fg);
                        bg = vterm_color_to_256(first_cell->bg);

                        /* handle reverse video attribute */
                        if (first_cell->attrs.reverse) {
                            int temp = fg;
                            fg = bg;
                            bg = temp;
                        }
                    }

                    /* collect consecutive cells with same colors */
                    int batch_start = col;
                    while (col < geo.width) {
                        int cell_fg = default_fg;
                        int cell_bg = default_bg;

                        if (col < line->cols) {
                            VTermScreenCell *cell_ptr = &line->cells[col];
                            cell_fg = vterm_color_to_256(cell_ptr->fg);
                            cell_bg = vterm_color_to_256(cell_ptr->bg);

                            /* handle reverse video attribute */
                            if (cell_ptr->attrs.reverse) {
                                int temp = cell_fg;
                                cell_fg = cell_bg;
                                cell_bg = temp;
                            }

                            /* break batch if colors changed */
                            if (cell_fg != fg || cell_bg != bg) {
                                break;
                            }

                            /* add character to buffer */
                            buf_idx += encode_utf8(cell_ptr->chars[0], &line_buf[buf_idx]);
                        } else {
                            /* past end of line, fill with spaces */
                            line_buf[buf_idx++] = ' ';
                        }

                        col++;
                    }

                    /* render this batch */
                    line_buf[buf_idx] = '\0';
                    int batch_width = col - batch_start;
                    Buffer_print(&main_buf, y, geo.x + batch_start, batch_width, line_buf, fg, bg);
                }
            }
        } else {
            /* this is a current screen line */
            int screen_row = virtual_line - vtd->scrollback.count;
            //LOG_INFO("current screen %d %d", screen_row, virtual_line);

            if (screen_row >= 0 && screen_row < vtd->rows) {
                /* render current screen line with color batching */
                int col = 0;
                while (col < geo.width) {
                    char line_buf[4096];
                    int buf_idx = 0;

                    /* get colors from first cell in batch */
                    pos.row = screen_row;
                    pos.col = col;
                    vterm_screen_get_cell(vtd->vts, pos, &cell);
                    int fg = vterm_color_to_256(cell.fg);
                    int bg = vterm_color_to_256(cell.bg);

                    /* handle reverse video attribute */
                    if (cell.attrs.reverse) {
                        int temp = fg;
                        fg = bg;
                        bg = temp;
                    }

                    /* collect consecutive cells with same colors */
                    int batch_start = col;
                    while (col < geo.width && col < vtd->cols) {
                        pos.row = screen_row;
                        pos.col = col;
                        vterm_screen_get_cell(vtd->vts, pos, &cell);

                        int cell_fg = vterm_color_to_256(cell.fg);
                        int cell_bg = vterm_color_to_256(cell.bg);

                        /* handle reverse video attribute */
                        if (cell.attrs.reverse) {
                            int temp = cell_fg;
                            cell_fg = cell_bg;
                            cell_bg = temp;
                        }

                        /* break batch if colors changed */
                        if (cell_fg != fg || cell_bg != bg) {
                            break;
                        }

                        /* add character to buffer */
                        buf_idx += encode_utf8(cell.chars[0], &line_buf[buf_idx]);

                        col++;
                    }

                    /* render this batch */
                    line_buf[buf_idx] = '\0';
                    int batch_width = col - batch_start;
                    Buffer_print(&main_buf, y, geo.x + batch_start, batch_width, line_buf, fg, bg);
                }
            }
        }
        if (line != NULL) line = line->next;
    }

    /* Render cursor if this terminal has focus */
    if (hasFocus) {
        VTermState *state = vterm_obtain_state(vtd->vt);
        VTermPos cursor_pos;
        vterm_state_get_cursorpos(state, &cursor_pos);

        /* Convert cursor position from screen coordinates to viewport coordinates */
        int cursor_virtual_line = vtd->scrollback.count + cursor_pos.row;
        int cursor_viewport_row = cursor_virtual_line - first_visible_line;

        /* Only draw cursor if it's visible in the viewport */
        if (cursor_viewport_row >= 0 && cursor_viewport_row < geo.height &&
            cursor_pos.col >= 0 && cursor_pos.col < geo.width) {
            int cursor_y = geo.y + cursor_viewport_row;
            int cursor_x = geo.x + cursor_pos.col;

            /* Get the character at cursor position */
            VTermScreenCell cell;
            vterm_screen_get_cell(vtd->vts, cursor_pos, &cell);

            int fg = vterm_color_to_256(cell.fg);
            int bg = vterm_color_to_256(cell.bg);

            /* Apply reverse video if already set */
            if (cell.attrs.reverse) {
                int temp = fg;
                fg = bg;
                bg = temp;
            }

            /* Render cursor - handle wide characters and continuation cells properly */
            char cursor_char[8];
            int cursor_width = 1;

            if (cell.chars[0] == (uint32_t)-1) {
                /* This is a continuation cell for a wide char, render space */
                strcpy(cursor_char, " ");
            } else if (cell.chars[0] == 0) {
                /* Empty cell */
                strcpy(cursor_char, " ");
            } else {
                /* Regular character - encode it */
                int len = encode_utf8(cell.chars[0], cursor_char);
                cursor_char[len] = '\0';
                /* Use the cell width from libvterm */
                cursor_width = cell.width > 0 ? cell.width : 1;
            }

            /* Render cursor with swapped colors (reverse video) */
            Buffer_print(&main_buf, cursor_y, cursor_x, cursor_width, cursor_char, bg, fg);
        }
    }
    update_tab_label(wg);
}


void vterm_send_key(struct Window *wg, char c)
{
    vterm_terminal_data *vtd = wg->data2;

    /*if (c == 12){ // Ctrl+K
      cycle_tabs();
      return;
    }*/


    /* just write to the PTY, the monitoring thread will handle reading the response */
    write(vtd->master, &c, 1);
    //TerminalWindow * terminal = wg;
    //update_tab_label(terminal);
}

void vterm_send_sequence(struct Window *wg, const char *seq, int len)
{
    vterm_terminal_data *vtd = wg->data2;

    /* log what we're sending */
    LOG_INFO("vterm_send_sequence: len=%d", len);
    for (int i = 0; i < len; i++) {
        LOG_INFO("  seq[%d] = %d (0x%02x) '%c'", i, (unsigned char)seq[i], (unsigned char)seq[i], seq[i] >= 32 ? seq[i] : '?');
    }

    /* parse escape sequence and convert to libvterm keyboard input */
    if (len >= 3 && seq[0] == 27 && seq[1] == '[') {
        /* standard CSI sequence */
        VTermKey key = VTERM_KEY_NONE;

        switch (seq[2]) {
            case 'A': key = VTERM_KEY_UP; break;
            case 'B': key = VTERM_KEY_DOWN; break;
            case 'C': key = VTERM_KEY_RIGHT; break;
            case 'D': key = VTERM_KEY_LEFT; break;
            case 'H': key = VTERM_KEY_HOME; break;
            case 'F': key = VTERM_KEY_END; break;
            default:
                /* unhandled escape sequence, write raw */
                LOG_INFO("unhandled CSI sequence, writing raw");
                write(vtd->master, seq, len);
                return;
        }

        if (key != VTERM_KEY_NONE) {
            LOG_INFO("sending vterm_keyboard_key: %d", key);
            vterm_keyboard_key(vtd->vt, key, VTERM_MOD_NONE);

            /* read the output generated by libvterm and write to PTY */
            char output[64];
            size_t output_len = vterm_output_read(vtd->vt, output, sizeof(output));
            if (output_len > 0) {
                LOG_INFO("vterm generated %zu bytes of output", output_len);
                write(vtd->master, output, output_len);
            }
            return;
        }
    }

    /* for non-escape sequences or unhandled ones, write raw */
    write(vtd->master, seq, len);
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



TerminalWindow *VTermTerminal_window(int initial_rows, int initial_cols)
{
    TerminalWindow *terminal = malloc(sizeof *terminal);
    Window_init(terminal, 0, 0, 0, 0, -1, -1);
    terminal->win.id = "vterm terminal window";
    terminal->win.send_key = vterm_send_key;
    terminal->win.send_sequence = vterm_send_sequence;

    terminal->win.draw = VTermTerminal_draw;
    terminal->win.scroll_wheel_up = VTermTerminal_scroll_wheel_up;
    terminal->win.scroll_wheel_down = VTermTerminal_scroll_wheel_down;

    vterm_terminal_data *vtd = malloc(sizeof *vtd);
    terminal->win.data2 = vtd;
    //frame->data2 = vtd;

    /* initialize libvterm */
    vtd->rows = initial_rows;
    vtd->cols = initial_cols;

    /* initialize scrollback list */
    vtd->scrollback.head = NULL;
    vtd->scrollback.tail = NULL;
    vtd->scrollback.count = 0;
    vtd->scrollback.max_size = 1000000;

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
        /* set TERM so programs like mc know what terminal type we are */
        setenv("TERM", "xterm-256color", 1);
        execl("/bin/sh", "sh", NULL);
        exit(1);
    }

    terminal->pid = pid;
    //terminal->cwd = get_shell_cwd_mac_native(pid);

    vtd->terminal = terminal;

    /* register this terminal for background monitoring */
    register_terminal(vtd);

    /* initial update to get the shell prompt */
    usleep(100000);  /* wait 100ms for initial shell output */
    VTermTerminal_update(vtd);

    return terminal;
}


Window *VTermTerminal_callback(){
    TerminalWindow *terminal = VTermTerminal_window(24, 80);
    Window *slider = slider_new(terminal);
    terminal->slider = slider;
    //slider->id = terminal->cwd;
    slider->id = malloc(1024);
    //Window_set_id_from_path(slider, terminal->cwd);
    update_tab_label(terminal);
    return slider;
}

Window *VTermTerminal_new_tab(TerminalFrame *self)
{

}

Window *VTermTerminal_menu(TerminalFrame *self)
{
    Window *menu = Menu_create_horizontal();

    Window *file = Menu_create_vertical(self);
    Menu_add_element(file, " 📄 New File   Ctrl+N", create_lambda(VTermTerminal_new_tab, 1, self));
    Menu_add_element(file, " 📁 New Folder Ctrl+N", create_lambda(VTermTerminal_new_tab, 1, self));
    Menu_add_element(file, "    New Window Ctrl+N", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(file, "    New Tab    Ctrl+N", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(file, "", NULL);
    Menu_add_element(file, " ❌ Close Window Ctrl+N", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(file, " ❌ Close Tab  Ctrl+W", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(file, "", NULL);
    Menu_add_submenu(menu, " File ", file);

    Window *edit = Menu_create_vertical(self);
    Menu_add_element(edit, " 🔪 Cut            Ctrl+X", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(edit, " 📋 Copy           Ctrl+C", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(edit, " 📌 Paste          Ctrl+V", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(edit, " ❌ Delete         Backspace", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(edit, " 📝 Rename         Ctrl+R", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(edit, "", NULL);
    Menu_add_element(edit, " 📋 Copy Name      Ctrl+C", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(edit, " 📋 Copy Directory Ctrl+C", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(edit, " 📋 Copy Path      Ctrl+C", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(edit, "", NULL);
    Menu_add_submenu(menu, " Edit ", edit);

    Window *view = Menu_create_vertical(self);
    //Menu_add_element(view, " ⤶ Word wrap", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(view, " ↓ Sort By Name",          create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(view, " ↓ Sort By Date Modified", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(view, " ↓ Sort By Size",          create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(view, "", NULL);
    Menu_add_submenu(menu, " View ", view);

    Menu_add_windows(menu, " Tabs ", self->tabs->data, self);

    return menu;
}

Window *VTermTerminal_toolbar(TerminalFrame *self)
{
    Window *toolbar = Menu_create_horizontal();
    Menu_add_element(toolbar, " + New Tab ", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(toolbar, " 🔪 Cut ", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(toolbar, " 📋 Copy ", create_lambda(VTermTerminal_new_tab, 0));
    Menu_add_element(toolbar, " 📌 Paste ", create_lambda(VTermTerminal_new_tab, 0));
    //Menu_add_element(toolbar, " 📝 Rename ", create_lambda(VTermTerminal_new_tab, 0));
    //Menu_add_element(toolbar, " 🔄 Refresh ", create_lambda(VTermTerminal_new_tab, 0));
    //Menu_add_element(toolbar, " 🔼 Up ", create_lambda(VTermTerminal_new_tab, 0));
    //Menu_add_element(toolbar, " 📝 Edit ", create_lambda(VTermTerminal_new_tab, 0));
    //Menu_add_element(toolbar, " 💻 Terminal ", create_lambda(VTermTerminal_new_tab, 0));

    toolbar->top = 1;

    return toolbar;
}

Window *VTermTerminal_new(int left, int right, int top, int bottom, int width, int height)
{
    TerminalFrame *frame = malloc(sizeof *frame);
    Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 1);
    //frame->send_key = vterm_send_key;
    //frame->send_sequence = vterm_send_sequence;

    // tabs
    Window *tabs = Tab_new(VTermTerminal_callback, 1);
    frame->tabs = tabs;
    tabs->top = 2;
    tabs->bottom = 0;
    tabs->left = 0;
    tabs->right = 0;
    Window_append(w, tabs);
    frame->win.focused = tabs;

    Window *toolbar = VTermTerminal_toolbar(frame);
    Window_append(w, toolbar);
    Window *menu = VTermTerminal_menu(frame);
    Window_append(w, menu);

    return frame;
}
