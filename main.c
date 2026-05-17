#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "ansi_term.c"
#include "logger.c"

int ends_with(const char *str, const char *suffix) {
    size_t len_str = strlen(str);
    size_t len_suf = strlen(suffix);

    if (len_str < len_suf) {
        return 0;
    }

    return strcmp(str + (len_str - len_suf), suffix) == 0;
}


// window operations

/*
typedef struct Window {
  struct Window* parent;
  struct Window* next;
  struct Window* prev;
  int x;
  int y;
  int width;
  int height;
  //Widget* wg_head;
  //Widget* wg_tail;
  void (*draw)(struct Window*);
} Window;

typedef struct Widget {
  Window win;
  //struct Widget* next;
  //struct Widget* prev;
  //int x;
  //int y;
  //int width;
  //int height;
  char * c;
  ForegroundColor fg;
  BackgroundColor bg;
  void (*on_mouse_down) (struct Widget* wg, int x, int y);  
} Widget;
*/

typedef struct Window {
  struct Window* parent;
  struct Window* head;
  struct Window* tail;
  struct Window* next;
  struct Window* prev;
  int x;
  int y;
  int width;
  int height;
  void (*draw)(struct Window*);
} Window;

typedef struct Widget {
  // for now we copy from Window
  struct Window* parent;
  struct Window* head;
  struct Window* tail;
  struct Window* next;
  struct Window* prev;
  int x;
  int y;
  int width;
  int height;
  void (*draw)(struct Window*);
  // for now we copy from Window
  
  char * c;
  ForegroundColor fg;
  BackgroundColor bg;
  void (*on_mouse_down) (struct Widget* wg, int x, int y);  
} Widget;

//Window* head = NULL;  // first node
//Window* tail = NULL;  // last node
Window* root = NULL;

Window* dragging = NULL;
Window* resizing = NULL;
int dragging_offset_x, dragging_offset_y;

/* Window */

/*
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
*/

void Window_append2(Window* w, Window* new_w){
  new_w->next = NULL;
  new_w->prev = w->tail;

  if (w->tail != NULL) {
	w->tail->next = new_w;
  } else {
	w->head = new_w;
  }

  w->tail = new_w;
}

/* Widget */

int Widget_get_width(Widget* wg){
  if (wg->width >= 0){
	return wg->width;
  }
  return wg->parent->width - wg->x;
}

int Widget_get_x(Widget* wg){
  if (wg->x >= 0){
	return wg->x;
  }
  return wg->parent->width + wg->x;
}

int Widget_get_y(Widget* wg){
  if (wg->y >= 0){
	return wg->y;
  }
  return wg->parent->height + wg->y + 1;
}

void Window_draw(struct Window* w){
  LOG_INFO("Window_draw");
  Window* current = w->head;
  while (current != NULL) {
	current->draw(current);
	current = current->next;
  }
  /*
  int x = w->x;
  int y = w->y;

  // draw widgets
  Widget* current = w->wg_head;
  while (current != NULL) {
	int wg_x = Widget_get_x(current);
	int wg_y = Widget_get_y(current);
	if (wg_y <= w->height) {
	  int wg_width = Widget_get_width(current);
	  set_terminal_color(current->fg, current->bg);
	  move_cursor(y+wg_y, x+wg_x);
	  if (wg_width > 0){
		for (int i = 0; i< wg_width; i++) printf(" ");
	  }
	  move_cursor(y+wg_y, x+wg_x);
	  printf(current->c);
	}
	current = current->next;
  }
  
  printf("\033[0m");
  fflush(stdout);
  */
}

void Widget_draw(struct Window* wg){
  Widget* current = (Widget*) wg;
  Window* w = current->parent;
  int x = w->x;
  int y = w->y;
  Window_draw((Window*)current);
  int wg_x = Widget_get_x(current);
  int wg_y = Widget_get_y(current);
  if (wg_y <= w->height) {
	int wg_width = Widget_get_width(current);
	set_terminal_color(current->fg, current->bg);
	move_cursor(y+wg_y, x+wg_x);
	if (wg_width > 0){
	  for (int i = 0; i< wg_width; i++) printf(" ");
	}
	move_cursor(y+wg_y, x+wg_x);
	printf(current->c);
  }
}

Window* Window_new(int x, int y, int width, int height){
  Window* w = malloc(sizeof *w);
  w->next = NULL;
  w->prev = NULL;
  
  w->x = x;
  w->y = y;
  w->width = width;
  w->height = height;
  //w->draw = draw;
  w->draw = Window_draw;


  return w;
}

int Widget_in_bounds(Widget* wg, int x, int y){
  int wg_x = wg->parent->x + Widget_get_x(wg);
  int wg_y = wg->parent->y + Widget_get_y(wg);
  
  if (wg_x <= x && x < wg_x + Widget_get_width(wg) &&
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
  wg->draw = Widget_draw;
  wg->parent = w;
  wg->x = x;
  wg->y = y;
  wg->width = width;
  wg->height = height;
  wg->c = c;
  wg->fg = fg;
  wg->bg = bg;
  //Widget_init(wg, x, y, width, c, fg, bg);

  Window_append2(w, wg);

  // add to list
  /*wg->next = NULL;
  wg->prev = w->wg_tail;

  if (w->wg_tail != NULL) {
	w->wg_tail->next = wg;
  } else {
	w->wg_head = wg;
  }

  w->wg_tail = wg;*/

  return wg;
}

void on_mouse_down_window_bar(Widget* wg, int x, int y){
  dragging = wg->parent;
  dragging_offset_x = x - wg->parent->x;
  dragging_offset_y = y - wg->parent->y;
}

void Window_add_window_bar(struct Window* w){
  Widget* wg = Window_add_widget(w, 0, 0, -1, 1, "", WHITE, BLUE_BG);
  wg->on_mouse_down = on_mouse_down_window_bar;
  
  Widget* close = Window_add_widget(w, -1, 0, 1, 1, "X", WHITE, BLUE_BG);
  Widget* maximize = Window_add_widget(w, -3, 0, 1, 1, "□", WHITE, BLUE_BG);
  Widget* minimize = Window_add_widget(w, -5, 0, 1, 1, "-", WHITE, BLUE_BG);
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

Widget* find_widget(Window* w, int x, int y){
  /*Window* w = tail;
  while (w != NULL) {
	
	Widget* wg = w->wg_tail;
	while (wg != NULL) {
	  //if (in_bounds(wg->x, wg->y, wg->width, wg->height, x, y)) return wg;
	  if (Widget_in_bounds(wg, x, y)) return wg;
	  wg = wg->prev;
	}
	
	w = w->prev;
	}*/
  return NULL;
}

// FILE MANAGER

void Widget_on_resize(Widget* wg, int x, int y){
  //LOG_INFO("Widget_on_resize");

  resizing = wg->parent;
  dragging_offset_x = x - wg->parent->width;
  dragging_offset_y = y - wg->parent->height;
}

Window* FileExplorer_new(int x, int y, int width, int height){
  //Window* w = malloc(sizeof *w);
  Window* w = Window_new(x, y, width, height);
  //w->draw = FileExplorer_draw;


  Window_add_window_bar(w);
  
  int j = 1;

  int widget_width;
  
  // menubar
  int x_offset = 0;
  widget_width = 6; Window_add_widget(w, x_offset, j, widget_width, 1, " File", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 6; Window_add_widget(w, x_offset, j, widget_width, 1, " Edit", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 6;  Window_add_widget(w, x_offset, j, widget_width, 1, " View", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 6;  Window_add_widget(w, x_offset, j, -1, 1, " Help", BLACK, WHITE_BG); x_offset += widget_width;
  //Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
  j++;

  // toolbar
  x_offset = 0;
  widget_width = 12; Window_add_widget(w, x_offset, j, widget_width, 1, "📄 New File", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 12; Window_add_widget(w, x_offset, j, widget_width, 1, "📁 New Dir", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 8;  Window_add_widget(w, x_offset, j, widget_width, 1, "📋 Copy", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 8;  Window_add_widget(w, x_offset, j, widget_width, 1, "🔪 Cut", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 10; Window_add_widget(w, x_offset, j, widget_width, 1, "📌 Paste", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 10; Window_add_widget(w, x_offset, j, widget_width, 1, "🔤 Rename", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 10; Window_add_widget(w, x_offset, j, -1, 1, "❌ Delete", BLACK, WHITE_BG); x_offset += widget_width;
  //Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
  j++;

  // tabs
  x_offset = 0;
  widget_width = 14; Window_add_widget(w, x_offset, j, widget_width, 1, "jordicolomer x", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 1;  Window_add_widget(w, x_offset, j, -1, 1, " + ", BLACK, WHITE_BG); x_offset += widget_width;
  //Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
  j++;

  // address bar
  Window_add_widget(w, 0, j, -1, 1, "📁 /Users/jordicolomer", BLACK, BRIGHT_BLUE_BG);
  j++;

  // favorites
  int start_j = j;
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
  while(j <= 200) Window_add_widget(w, 0, j++, fav_width, 1, "", BLACK, WHITE_BG);

  // list files
  j = start_j;
  DIR *dir;
  struct dirent *entry;

  dir = opendir("/Users/jordicolomer");  // current directory

  if (dir == NULL) {
	perror("opendir");
	return NULL;
  }

  //int x = 0;
  while ((entry = readdir(dir)) != NULL) {
	char * icon = "📄";
	if (entry->d_type == DT_DIR) {
	  icon = "📁";
	}
	if (ends_with(entry->d_name, ".png")){icon = "🖼️";}
	if (ends_with(entry->d_name, ".pdf")){icon = "📖";}
	char *str = NULL;
	// memory leak here
	int len = asprintf(&str, "%s %s", icon, entry->d_name);
	Window_add_widget(w, fav_width, j++, -1, 1, str, BLACK, WHITE_BG);
	//if (height < j) break;
	//mvwprintw(win, x++, 1, "%s %s", icon, entry->d_name);
  }

  closedir(dir);

  // resize grip
  Widget* resize_grip = Window_add_widget(w, -1, -1, 1, 1, "⌟", BLACK, WHITE_BG);
  resize_grip->on_mouse_down = Widget_on_resize;

  return w;
}

// TERMINAL

void init(){
  //Window* w1 = append_window(10, 20, 30, 40, draw_file_explorer);
  //Window* w2 = append_window(100, 30, 30, 40, draw_file_explorer);
  root = Window_new(0, 0, 200, 200);
  
  Window* w1 = FileExplorer_new(10, 20, 80, 20);
  Window_append2(root, w1);
  
  Window* w2 = FileExplorer_new(100, 30, 80, 40);
  Window_append2(root, w2);
  
  dragging = w2;
}


void repaint(){
  clear_screen();
  hide_cursor();
  
  // redraw everything
  root->draw(root);
  /*Window* win = tail;
  while (win != NULL) {
	win->draw(win);
	win = win->prev;
	}*/
}

void on_drag(int x, int y){
  if (dragging != NULL){
	dragging->x = x - dragging_offset_x;
	dragging->y = y - dragging_offset_y;
  }
  if (resizing != NULL){
	resizing->width = x - dragging_offset_x;
	resizing->height = y - dragging_offset_y;
	LOG_INFO("new dimensions width: %d height: %d", resizing->width, resizing->height);	
  }

  repaint();
}

void on_mouse_down(int x, int y){
  Widget* wg = find_widget(root, x, y);


  if (wg != NULL){
	if (wg->on_mouse_down != NULL) wg->on_mouse_down(wg, x, y);
    /*LOG_INFO("Found widget %p | pos(%d,%d) size(%dx%d)", 
             (void*)wg, 
             wg->x, wg->y, 
             wg->width, wg->height);*/
  }
  //on_drag(x, y);
}

void on_mouse_up(){
  dragging = NULL;
  resizing = NULL;
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
