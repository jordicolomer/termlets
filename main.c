#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include "ansi_term.c"


// window operations
typedef struct Widget {
  struct Window* parent;
  struct Widget* next;
  struct Widget* prev;
  int x;
  int y;
  int width;
  int height;
  char * c;
  ForegroundColor fg;
  BackgroundColor bg;
  void (*on_mouse_down) (struct Widget* wg, int x, int y);  
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
int dragging_offset_x, dragging_offset_y;

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


void Window_draw(struct Window* w){
  int x = w->x;
  int y = w->y;

  // draw widgets
  Widget* current = w->wg_tail;
  while (current != NULL) {
	set_terminal_color(current->fg, current->bg);
	move_cursor(y+current->y, x+current->x);
	if (current->width > 0){
	  for (int i=0;i<current->width;i++) printf(" ");
	}
	move_cursor(y+current->y, x+current->x);
	printf(current->c);
	current = current->prev;
  }
  
  printf("\033[0m");
  fflush(stdout);
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
  w->draw = Window_draw;

  Window_append(w);

  return w;
}

int Widget_in_bounds(Widget* wg, int x, int y){
  int wg_x = wg->parent->x + wg->x;
  int wg_y = wg->parent->y + wg->y;
  
  if (wg_x <= x && x < wg_x + wg->width &&
	  wg_y <= y && y < wg_y + wg->height) {
	return 1;
  }
  return 0;
}

/*void Widget_init(Widget* wg, int x, int y, int width, char * c, ForegroundColor fg, BackgroundColor bg){
  wg->x = x;
  wg->y = y;
  wg->width = width;
  wg->c = c;
  wg->fg = fg;
  wg->bg = bg;
  }*/

Widget* Window_add_widget(Window* w, int x, int y, int width, int height, char * c, ForegroundColor fg, BackgroundColor bg){
  Widget* wg = malloc(sizeof *wg);
  wg->parent = w;
  wg->x = x;
  wg->y = y;
  wg->width = width;
  wg->height = height;
  wg->c = c;
  wg->fg = fg;
  wg->bg = bg;
  //Widget_init(wg, x, y, width, c, fg, bg);

  // add to list
  wg->next = NULL;
  wg->prev = w->wg_tail;

  if (w->wg_tail != NULL) {
	w->wg_tail->next = wg;
  } else {
	w->wg_head = wg;
  }

  w->wg_tail = wg;

  return wg;
}

void on_mouse_down_window_bar(Widget* wg, int x, int y){
  dragging = wg->parent;
  dragging_offset_x = x - wg->parent->x;
  dragging_offset_y = y - wg->parent->y;
}

void Window_add_window_bar(struct Window* w){
  Widget* wg = Window_add_widget(w, 0, 0, w->width, 1, "", WHITE, BLUE_BG);
  wg->on_mouse_down = on_mouse_down_window_bar;
}

/*
int in_bounds(int x, int y, int width, int height, int point_x, int point_y) {
    if (x <= point_x && point_x < x + width &&
        y <= point_y && point_y < y + height) {
        return 1;
    }
    return 0;
}
*/

Widget* find_widget(int x, int y){
  Window* w = tail;
  while (w != NULL) {
	
	Widget* wg = w->wg_tail;
	while (wg != NULL) {
	  //if (in_bounds(wg->x, wg->y, wg->width, wg->height, x, y)) return wg;
	  if (Widget_in_bounds(wg, x, y)) return wg;
	  wg = wg->prev;
	}
	
	w = w->prev;
  }
  return NULL;
}

// FILE MANAGER


Window* FileExplorer_new(int x, int y, int width, int height){
  //Window* w = malloc(sizeof *w);
  Window* w = Window_new(x, y, width, height);
  //w->draw = FileExplorer_draw;

  Window_add_window_bar(w);
  int j = 1;
  int fav_width = 22;
  Window_add_widget(w, 0, j++, fav_width, 1, " Favorites", WHITE, BRIGHT_BLACK_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, " 🏠 Home", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, " 📥 Downloads", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, " 📄 Documents", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, " 📷 Pictures", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, " 🎵 Music", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, " 🎬 Movies", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, "", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, " Locations", WHITE, BRIGHT_BLACK_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, " 💻 Root", BLACK, WHITE_BG);
  Window_add_widget(w, 0, j++, fav_width, 1, " 👥 Users", BLACK, WHITE_BG);

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

#include "logger.c"

void repaint(){
  clear_screen();
  hide_cursor();
  
  // redraw everything
  Window* win = tail;
  while (win != NULL) {
	win->draw(win);
	win = win->prev;
  }
}

void on_drag(int x, int y){
  if (dragging != NULL){
	dragging->x = x - dragging_offset_x;
	dragging->y = y - dragging_offset_y;
  }

  repaint();
}

void on_mouse_down(int x, int y){
  Widget* wg = find_widget(x, y);


  if (wg != NULL){
	if (wg->on_mouse_down != NULL) wg->on_mouse_down(wg, x, y);
    /*LOG_INFO("Found widget %p | pos(%d,%d) size(%dx%d)", 
             (void*)wg, 
             wg->x, wg->y, 
             wg->width, wg->height);*/
  }
  on_drag(x, y);
}

void on_mouse_up(){
  dragging = NULL;
}

//#include <locale.h>
int main() {
  log_init("app.log");
  //log_set_level(LOG_DEBUG);
  //setlocale(LC_ALL, "");
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
		  on_mouse_down(x, y);
		}

		if (type == 'm') { //release
		  dragging = 0;
		  on_mouse_up();
		}
			  
		if (dragging == 1){
		  //repaint();
		  on_drag(x, y);
		}
	  }
	}
  }

  disable_mouse();
  disable_raw_mode();

  printf("\033[2J\033[H");
  return 0;
}
