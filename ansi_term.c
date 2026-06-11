#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include "ansi_term.h"

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

  raw.c_lflag &= ~(ICANON | ECHO | ISIG);
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

void cleanup(void) {
    // Disable mouse tracking modes
    printf("\x1b[?1000l"); // normal mouse tracking
    printf("\x1b[?1002l"); // button-event tracking
    printf("\x1b[?1003l"); // any-event tracking
    printf("\x1b[?1006l"); // SGR extended mode
    printf("\x1b[?1015l"); // urxvt mode

    // Show cursor
    printf("\x1b[?25h");

    // Leave alternate screen buffer
    printf("\x1b[?1049l");

    // Reset attributes/colors
    printf("\x1b[0m");

    fflush(stdout);
}

void enter_alternate_screen(void) {
      // Enter alternate screen
    printf("\x1b[?1049h");

    // Hide cursor
    //printf("\x1b[?25l");

    // Blue background + white text
    //printf("\x1b[44m\x1b[37m");
	//set_color256(232, 32);

    // Clear + home
    //printf("\x1b[2J\x1b[H");

    fflush(stdout);
}

