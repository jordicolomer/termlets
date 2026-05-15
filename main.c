#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include "ansi_term.c"

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

// window operations
typedef struct Widget {
  struct Widget* next;
  struct Widget* prev;
  int x;
  int y;
  int width;
  char * c;
  ForegroundColor fg;
  BackgroundColor bg;
} Widget;

typedef struct Window {
  struct Window* next;
  struct Window* prev;
  int x;
  int y;
  int width;
  int height;
  Widget* wg_head;
  Widget* wg_tail;
  void (*draw)(struct Window*);
} Window;

Window* head = NULL;  // first node
Window* tail = NULL;  // last node
Window* dragging = NULL;

void Window_append(Window* w){
  w->next = NULL;
  w->prev = tail;   // link back to old last node

  if (tail != NULL) {
	tail->next = w;   // link old tail forward
  } else {
	// list was empty
	head = w;
  }

  tail = w;  // new node becomes the last node
}

Window* Window_new(int x, int y, int width, int height){
  Window* w = malloc(sizeof *w);
  w->wg_head = NULL;
  w->wg_tail = NULL;
  
  w->x = x;
  w->y = y;
  w->width = width;
  w->height = height;
  //w->draw = draw;

  Window_append(w);

  return w;
}

void Widget_init(Widget* wg, int x, int y, int width, char * c, ForegroundColor fg, BackgroundColor bg){
  wg->x = x;
  wg->y = y;
  wg->width = width;
  wg->c = c;
  wg->fg = fg;
  wg->bg = bg;
}

void Window_add_widget(Window* w, int x, int y, int width, char * c, ForegroundColor fg, BackgroundColor bg){
  Widget* wg = malloc(sizeof *wg);
  Widget_init(wg, x, y, width, c, fg, bg);

  // add to list
  wg->next = NULL;
  wg->prev = w->wg_tail;

  if (w->wg_tail != NULL) {
	w->wg_tail->next = wg;
  } else {
	w->wg_head = wg;
  }

  w->wg_tail = wg;
}

// FILE MANAGER

void FileExplorer_draw(struct Window* w){
  int x = w->x;
  int y = w->y;

  // draw widgets
  Widget* current = w->wg_tail;
  while (current != NULL) {
	set_terminal_color(current->fg, current->bg);
	move_cursor(y+current->y, x+current->x);
	printf(current->c);
	if (current->width > 0){
	  for (int i=0;i<current->width;i++) printf(" ");
	}
	current = current->prev;
  }
  
  printf("\033[0m");
  fflush(stdout);
}

void Window_add_window_bar(struct Window* w){
  Window_add_widget(w, 0, 0, w->width, "", WHITE, BLUE_BG);  
}

Window* FileExplorer_new(int x, int y, int width, int height){
  //Window* w = malloc(sizeof *w);
  Window* w = Window_new(x, y, width, height);
  w->draw = FileExplorer_draw;

  Window_add_window_bar(w);
  int j = 1;
  Window_add_widget(w, 0, j++, 0, " Favorites           ", WHITE, BRIGHT_BLACK_BG);
  Window_add_widget(w, 0, j++, 0, " 🏠 Home             ", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, 0, " 📥 Downloads        ", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, 0, " 📄 Documents        ", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, 0, " 📷 Pictures         ", BLACK, WHITE_BG);

  return w;
}

// TERMINAL

void init(){
  //Window* w1 = append_window(10, 20, 30, 40, draw_file_explorer);
  //Window* w2 = append_window(100, 30, 30, 40, draw_file_explorer);
  Window* w1 = FileExplorer_new(10, 20, 30, 40);
  Window* w2 = FileExplorer_new(100, 30, 30, 40);
  dragging = w2;
}


void on_click(int x, int y){
  clear_screen();
  hide_cursor();

  if (dragging != NULL){
	dragging->x = x;
	dragging->y = y;
  }

  Window* current = tail;
  while (current != NULL) {
	current->draw(current);  // or whatever you want to do	
	current = current->prev;
  }
}

int main() {
  enable_raw_mode();
  enable_mouse();
  init();
	
  //printf("\033[2J\033[H");
  //printf("Move mouse / click / drag. Press 'q' to quit.\n");
  int dragging = 0;

  while (1) {
	char c;
	read(STDIN_FILENO, &c, 1);

	if (c == 'q') break;

	// ESC sequence
	if (c == 27) {
	  char seq[32];
	  int i = 0;

	  // read rest of escape sequence
	  while (i < 31 && read(STDIN_FILENO, &seq[i], 1) == 1) {
		if (seq[i] == 'm' || seq[i] == 'M') {
		  i++;
		  break;
		}
		i++;
	  }

	  seq[i] = 0;

	  int btn, x, y;
	  char type;

	  if (sscanf(seq, "[<%d;%d;%d%c", &btn, &x, &y, &type) == 4) {
		if (dragging == 0 && btn == 0 && type == 'M') { //click
		  dragging = 1;
		}

		if (type == 'm') { //release
		  dragging = 0;
		}
			  
		if (dragging == 1){
		  on_click(x, y);
		}
	  }
	}
  }

  disable_mouse();
  disable_raw_mode();

  printf("\033[2J\033[H");
  return 0;
}
