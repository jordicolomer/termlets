#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

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
typedef struct Window {
  struct Window* next;
  struct Window* prev;
  int x;
  int y;
  int width;
  int height;
  void (*draw)(struct Window*);
} Window;

Window* head = NULL;  // first node
Window* tail = NULL;  // last node

void append_window(int x, int y, int width, int height, void (*draw)(struct Window*)){
  //Window* w = malloc(sizeof(Window));
  Window* w = malloc(sizeof *w);
  
  w->x = x;
  w->y = y;
  w->width = width;
  w->height = height;
  w->draw = draw;

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

void draw_file_explorer(struct Window* w){
  int x = w->x;
  int y = w->y;
  printf("\033[37;44m"); // white on blue background
  move_cursor(y, x);
  printf("                     ");
  printf("\033[30;47m"); // black text on white background
  move_cursor(y+1, x);
  printf(" Favorites           ");
  move_cursor(y+2, x);
  printf(" 🏠 Home             ");
  move_cursor(y+3, x);
  printf(" 📥 Downloads        ");
  move_cursor(y+4, x);
  printf(" 📄 Documents        ");
  move_cursor(y+5, x);
  printf(" 📷 Pictures         ");
  printf("\033[0m");
  fflush(stdout);
}

void init(){
  append_window(10, 20, 30, 40, draw_file_explorer);
  append_window(20, 30, 30, 40, draw_file_explorer);
}


void draw(int x, int y){
  clear_screen();
  hide_cursor();
  if (head != 0){
	head->x = x;
	head->y = y;
  }
  Window* current = head;

  while (current != NULL) {
	current->draw(current);  // or whatever you want to do
	
	current = current->next;
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
		  draw(x, y);
		}
	  }
	}
  }

  disable_mouse();
  disable_raw_mode();

  printf("\033[2J\033[H");
  return 0;
}
