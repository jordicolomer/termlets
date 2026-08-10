#ifndef ANSI_TERM_H
#define ANSI_TERM_H

typedef enum {
    BLACK = 0,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,

    DEFAULT_COLOR = 39,

    BRIGHT_BLACK = 90,
    BRIGHT_RED = 91,
    BRIGHT_GREEN = 92,
    BRIGHT_YELLOW = 93,
    BRIGHT_BLUE = 94,
    BRIGHT_MAGENTA = 95,
    BRIGHT_CYAN = 96,
    BRIGHT_WHITE = 97
} ForegroundColor;

typedef enum {
    BLACK_BG = 40,
    RED_BG = 41,
    GREEN_BG = 42,
    YELLOW_BG = 43,
    BLUE_BG = 27,
    MAGENTA_BG = 45,
    CYAN_BG = 46,
    WHITE_BG = 15,

    DEFAULT_BG = 49,

    BRIGHT_BLACK_BG = 100,
    BRIGHT_RED_BG = 101,
    BRIGHT_GREEN_BG = 102,
    BRIGHT_YELLOW_BG = 103,
    BRIGHT_BLUE_BG = 104,
    BRIGHT_MAGENTA_BG = 105,
    BRIGHT_CYAN_BG = 106,
    BRIGHT_WHITE_BG = 107
} BackgroundColor;

void set_terminal_color(ForegroundColor fg, BackgroundColor bg);
void set_terminal_fg_color256(int color);
void set_terminal_bg_color256(int color);
void set_color256(int fg, int bg);
void reset_terminal_color(void);
void enable_raw_mode();
void disable_raw_mode();
void enable_mouse();
void disable_mouse();
void move_cursor(int row, int col);
void clear_screen();
void hide_cursor();
void show_cursor();
void get_terminal_size(int *rows, int *cols);
void cleanup(void);
void enter_alternate_screen(void);

/* Platform-agnostic input functions */
int check_input_available(int timeout_usec);
int read_char(char *c);

#endif // ANSI_TERM_H
