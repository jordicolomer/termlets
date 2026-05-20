#ifndef ANSI_TERM_H
#define ANSI_TERM_H

#include <sys/ioctl.h>
#include <unistd.h>

/**
 * set_terminal_color
 * ------------------
 * Sets ANSI terminal foreground/background colors.
 *
 * Usage:
 *     set_terminal_color(fg, bg);
 *
 * Example:
 *     set_terminal_color(WHITE, BLUE);
 *
 * Resets:
 *     reset_terminal_color();
 *
 * Foreground colors:
 *     BLACK         = 30
 *     RED           = 31
 *     GREEN         = 32
 *     YELLOW        = 33
 *     BLUE          = 34
 *     MAGENTA       = 35
 *     CYAN          = 36
 *     WHITE         = 37
 *     DEFAULT_COLOR = 39
 *
 * Background colors:
 *     BLACK_BG      = 40
 *     RED_BG        = 41
 *     GREEN_BG      = 42
 *     YELLOW_BG     = 43
 *     BLUE_BG       = 44
 *     MAGENTA_BG    = 45
 *     CYAN_BG       = 46
 *     WHITE_BG      = 47
 *     DEFAULT_BG    = 49
 *
 * Bright foreground colors:
 *     BRIGHT_BLACK   = 90
 *     BRIGHT_RED     = 91
 *     BRIGHT_GREEN   = 92
 *     BRIGHT_YELLOW  = 93
 *     BRIGHT_BLUE    = 94
 *     BRIGHT_MAGENTA = 95
 *     BRIGHT_CYAN    = 96
 *     BRIGHT_WHITE   = 97
 *
 * Bright background colors:
 *     BRIGHT_BLACK_BG   = 100
 *     BRIGHT_RED_BG     = 101
 *     BRIGHT_GREEN_BG   = 102
 *     BRIGHT_YELLOW_BG  = 103
 *     BRIGHT_BLUE_BG    = 104
 *     BRIGHT_MAGENTA_BG = 105
 *     BRIGHT_CYAN_BG    = 106
 *     BRIGHT_WHITE_BG   = 107
 */

#include <stdio.h>

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

void set_terminal_color(ForegroundColor fg, BackgroundColor bg) {
    printf("\033[%d;%dm", fg, bg);
}

void set_terminal_fg_color256(int color) {
    printf("\x1b[38;5;%dm", color);
}
void set_terminal_bg_color256(int color) {
    printf("\x1b[48;5;%dm", color);
}
void set_color256(int fg, int bg) {
    printf("\x1b[38;5;%d;48;5;%dm", fg, bg);
}

void reset_terminal_color(void) {
    printf("\033[0m");
}

int test() {
    set_terminal_color(WHITE, BLUE_BG);
    printf("White on blue\n");

    set_terminal_color(BLACK, WHITE_BG);
    printf("Black on white\n");

    set_terminal_color(BRIGHT_YELLOW, RED_BG);
    printf("Bright yellow on red\n");

    reset_terminal_color();

    return 0;
}


struct termios orig;

void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &orig);
  struct termios raw = orig;

  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void enable_mouse() {
  // enable SGR mouse tracking + movement
  printf("\033[?1000h"); // clicks
  printf("\033[?1003h"); // movement tracking
  printf("\033[?1006h"); // SGR extended mode
  fflush(stdout);
}

void disable_mouse() {
  printf("\033[?1003l");
  printf("\033[?1000l");
  fflush(stdout);
}

void move_cursor(int row, int col) {
  printf("\033[%d;%dH", row, col);
  fflush(stdout);
}


void clear_screen() {
  printf("\033[2J");
}

void hide_cursor() {
  printf("\033[?25l");
  fflush(stdout);
}

void show_cursor() {
  printf("\033[?25h");
  fflush(stdout);
}

void get_terminal_size(int *rows, int *cols) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *cols = w.ws_col;
    *rows = w.ws_row;
}

#endif // ANSI_TERM_H
