#include <stdio.h>
#include "ansi_term.h"

#ifdef _WIN32
    #define NOMINMAX  /* Prevent Windows from defining min/max macros */
    #include <windows.h>
    #include <io.h>
    #include <conio.h>
    #define STDIN_FILENO 0
    #define STDOUT_FILENO 1
#else
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <termios.h>
#endif

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


#ifdef _WIN32
static DWORD orig_input_mode;
static DWORD orig_output_mode;
#else
static struct termios orig;
#endif

void enable_raw_mode() {
#ifdef _WIN32
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

  GetConsoleMode(hIn, &orig_input_mode);
  GetConsoleMode(hOut, &orig_output_mode);

  DWORD mode = orig_input_mode;
  mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
  mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
  SetConsoleMode(hIn, mode);

  DWORD out_mode = orig_output_mode;
  out_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
  SetConsoleMode(hOut, out_mode);
#else
  tcgetattr(STDIN_FILENO, &orig);
  struct termios raw = orig;

  /* disable input processing that could interfere with escape sequences */
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  /* disable output processing */
  raw.c_oflag &= ~(OPOST);
  /* disable canonical mode, echo, and signals */
  raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif
}

void disable_raw_mode() {
#ifdef _WIN32
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  SetConsoleMode(hIn, orig_input_mode);
  SetConsoleMode(hOut, orig_output_mode);
#else
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
#endif
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
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *cols = w.ws_col;
    *rows = w.ws_row;
#endif
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

/* Platform-agnostic input functions */
int check_input_available(int timeout_usec) {
#ifdef _WIN32
    /* Windows: use _kbhit() for checking input, Sleep for timeout */
    if (_kbhit()) {
        return 1;
    }
    if (timeout_usec > 0) {
        Sleep(timeout_usec / 1000);  /* Sleep takes milliseconds */
    }
    return _kbhit();
#else
    /* POSIX: use select() */
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval tv;
    tv.tv_sec = timeout_usec / 1000000;
    tv.tv_usec = timeout_usec % 1000000;

    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
#endif
}

int read_char(char *c) {
#ifdef _WIN32
    /* Windows: use _getch() which doesn't echo */
    if (_kbhit()) {
        *c = _getch();
        return 1;
    }
    return 0;
#else
    /* POSIX: use read() */
    return read(STDIN_FILENO, c, 1) == 1;
#endif
}

