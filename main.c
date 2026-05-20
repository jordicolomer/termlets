#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "ansi_term.c"
#include "logger.c"

//#define USE_BUFFER

/* utils.c */

int ends_with(const char *str, const char *suffix) {
    size_t len_str = strlen(str);
    size_t len_suf = strlen(suffix);

    if (len_str < len_suf) {
        return 0;
    }

    return strcmp(str + (len_str - len_suf), suffix) == 0;
}

/* buffer.c */
#include "buffer.c"


/* window.c */

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
  void (*draw)(struct Window*, int, int);
  void (*on_mouse_down) (struct Widget* wg, int x, int y);  
} Window;

Window* root = NULL;

Window* dragging = NULL;
Window* resizing = NULL;
int dragging_offset_x, dragging_offset_y;


void Window_append(Window* w, Window* new_w){
  new_w->parent = w;
  new_w->next = NULL;
  new_w->prev = w->tail;

  if (w->tail != NULL) {
	w->tail->next = new_w;
  } else {
	w->head = new_w;
  }

  w->tail = new_w;
}

int Window_get_width(Window* wg){
  if (wg->width >= 0){
	return wg->width;
  }
  return wg->parent->width - wg->x;
}

int Window_get_x(Window* wg){
  //LOG_INFO("Window_get_x");
  if (wg->x >= 0){
	//LOG_INFO("if");
	return wg->x;
  }
  //LOG_INFO("else");
  return wg->parent->width + wg->x;
}

int Window_get_y(Window* wg){
  if (wg->y >= 0){
	return wg->y;
  }
  return wg->parent->height + wg->y + 1;
}

int Window_get_absolute_x(Window* w){
  if (w == NULL) return 0;
  int x = Window_get_x(w);
  int x_parent = Window_get_absolute_x(w->parent);
  return x_parent+x;
}

int Window_get_absolute_y(Window* w){
  if (w == NULL) return 0;
  int y = Window_get_y(w);
  int y_parent = Window_get_absolute_y(w->parent);
  return y_parent+y;
}


void Window_draw(struct Window* w, int bias_x, int bias_y){
  Window* current = w->head;
  while (current != NULL) {
	current->draw(current, bias_x + w->x, bias_y + w->y);
	current = current->next;
  }
  //printf("\033[0m");
  //fflush(stdout);
}

Window* Window_init(Window* w, int x, int y, int width, int height){
  //Window* w = malloc(sizeof *w);
  w->head = NULL;
  w->tail = NULL;
  w->next = NULL;
  w->prev = NULL;
  
  w->x = x;
  w->y = y;
  w->width = width;
  w->height = height;
  //w->draw = draw;
  w->draw = Window_draw;
  w->parent = NULL;


  return w;
}

int Window_in_bounds(Window* wg, int x, int y){
  //LOG_INFO("Window_in_bounds %d", wg->x);
  int wg_x = Window_get_x(wg);
  //int wg_x = wg->parent->x + Window_get_x(wg);
  //LOG_INFO("wg_x = %d", wg_x);
  int wg_y = Window_get_y(wg);
  //int wg_y = wg->parent->y + Window_get_y(wg);

  //printf("Window_in_bounds: %d <= %d < %d and %d <= %d < %d\n", wg_x, x, wg_x + Window_get_width(wg), wg_y, y, wg_y + wg->height);
  if (wg_x <= x && x < wg_x + Window_get_width(wg) &&
	  wg_y <= y && y < wg_y + wg->height) {
	//LOG_INFO("Window_in_bounds: 1");
	return 1;
  }
  return 0;
}

Window* Window_find_widget(Window* this, int x, int y){
  LOG_INFO("Window_find_widget: %p", this);
  if (!this) return NULL;
  Window* ret = NULL;
  if (Window_in_bounds(this, x, y)){
	ret = this;
	LOG_INFO("ret: %p", ret);
  }
  Window* child = this->tail;
  while (child != NULL) {
	LOG_INFO("child: %p", child);
	Window* found = Window_find_widget(child,  x - this->x,  y - this->y);
	if (found != NULL) {
		return found;
	}
	child = child->prev;
  }
  return ret;
}

void Window_bring_to_bottom(Window* this) {
	LOG_INFO("Window_bring_to_bottom: %p", (void*)this->parent);
    if (!this || !this->parent) return;

    Window* parent = this->parent;

    // If already the tail, nothing to do
    if (parent->tail == this) return;

    // ---- 1. Unlink from current position ----
    if (this->prev) {
        this->prev->next = this->next;
    } else {
        // this was head
        parent->head = this->next;
    }

    if (this->next) {
        this->next->prev = this->prev;
    } else {
        // this was tail (redundant check, but safe)
        parent->tail = this->prev;
    }

    // ---- 2. Insert at tail ----
    this->next = NULL;
    this->prev = parent->tail;

    if (parent->tail) {
        parent->tail->next = this;
    }

    parent->tail = this;

    // If list was empty or had one element
    if (parent->head == NULL) {
        parent->head = this;
    }
}

/* widget.c */

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
  void (*draw)(struct Window*, int, int);
  void (*on_mouse_down) (struct Widget* wg, int x, int y);  
  // for now we copy from Window
  
  char * c;
  int fg;
  int bg;
  //int _fg;
  //int _bg;
} Widget;


void Widget_draw(struct Window* wg, int bias_x, int bias_y){
  Widget* current = (Widget*) wg;
  Window* w = current->parent;
  //int x = w->x;
  //int y = w->y;
  //int x = Window_get_absolute_x(w);
  //int y = Window_get_absolute_y(w);
  Window_draw((Window*)current, bias_x, bias_y);
  int wg_x = Window_get_x(current);
  int wg_y = Window_get_y(current);
  if (wg_y <= w->height && wg_x < w->width) {
	int wg_width = Window_get_width(current);
	/*set_terminal_color(current->fg, current->bg);
	move_cursor(y + wg_y, x + wg_x);
	if (wg_width > 0){
	  for (int i = 0; i< wg_width; i++) printf(" ");
	}
	move_cursor(bias_y + wg_y, bias_x + wg_x);
	printf(current->c);*/
#ifdef USE_BUFFER
	Buffer_print(&main_buf, bias_y + wg_y, bias_x + wg_x, wg_width, current->c, current->fg, current->bg);
#else
	Buffer_print_raw(&main_buf, bias_y + wg_y, bias_x + wg_x, wg_width, current->c, current->fg, current->bg);
	fprintf(stdout, "\033[0m");
	fflush(stdout);

#endif
	LOG_INFO("Widget_draw: %s %d %d", current->c, bias_y + wg_y, bias_x + wg_x);
  }
}



Widget* Window_add_widget(Window* w, int x, int y, int width, int height, char * c, int fg, int bg){
  Widget* wg = malloc(sizeof *wg);
  Window_init(wg, x, y, width, height);
  wg->draw = Widget_draw;
  wg->parent = w;
  wg->c = c;
  wg->fg = fg;
  wg->bg = bg;
  //wg->_fg = 0;
  //wg->_bg = 0;

  Window_append(w, wg);

  return wg;
}

/* frame.c */


void on_mouse_down_window_bar(Widget* wg, int x, int y){
  LOG_INFO("on_mouse_down_window_bar");
  dragging = wg->parent;
  Window_bring_to_bottom(dragging);
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

void Widget_on_resize(Widget* wg, int x, int y){
  LOG_INFO("Widget_on_resize");

  resizing = wg->parent;
  dragging_offset_x = x - wg->parent->width;
  dragging_offset_y = y - wg->parent->height;
}

Window* Frame_init(Window* w, int x, int y, int width, int height){
  Window_init(w, x, y, width, height);
  Window_add_window_bar(w);

  // add right border
  //for (int _ = 1; _ < w->height; _++) Window_add_widget(w, -1, _, 1, 1, "▊", WHITE, BLUE_BG);
  /*
  for (int _ = 1; _ < 1000; _++) Window_add_widget(w, -1, _, 1, 1, "▐", WHITE_BG, BLUE);
  for (int _ = 1; _ < 1000; _++) Window_add_widget(w, _, -1, 1, 1, "▄", WHITE_BG, BLUE);  
  for (int _ = 1; _ < 1000; _++) Window_add_widget(w, 0, _, 1, 1, "▌", WHITE_BG, BLUE);
  Window_add_widget(w, 0, -1, 1, 1, "▙", WHITE_BG, BLUE);  
  */
  /*
  for (int _ = 1; _ < 1000; _++) Window_add_widget(w, -1, _, 1, 1, " ", BLUE_BG, WHITE);
  for (int _ = 1; _ < 1000; _++) Window_add_widget(w, _, -1, 1, 1, " ", BLUE_BG, WHITE);  
  for (int _ = 1; _ < 1000; _++) Window_add_widget(w, 0, _, 1, 1, " ", BLUE_BG, WHITE);
  Window_add_widget(w, 0, -1, 1, 1, " ", BLUE_BG, WHITE);
  */
  /*
  for (int _ = 1; _ < 1000; _++) Window_add_widget(w, -1, _, 1, 1, "│", WHITE_BG, BLUE);
  for (int _ = 1; _ < 1000; _++) Window_add_widget(w, _, -1, 1, 1, "─", WHITE_BG, BLUE);  
  for (int _ = 1; _ < 1000; _++) Window_add_widget(w, 0, _, 1, 1, "│", WHITE_BG, BLUE);
  Window_add_widget(w, 0, -1, 1, 1, "└", WHITE_BG, BLUE);
  */
  
  // resize grip
  //Widget* resize_grip = Window_add_widget(w, -1, -1, 1, 1, "⌟", BLACK, WHITE_BG);
  Widget* resize_grip = Window_add_widget(w, -1, -1, 1, 1, "▟", WHITE_BG, BLUE);
  resize_grip->on_mouse_down = Widget_on_resize;

  Window* child = malloc(sizeof *child);
  //Window_init(child, 1, 1, width-2, height-2);
  Window_init(child, 0, 1, width, height);
  Window_append(w, child);

  return child;
}

/* file_manager.c */


Window* FileExplorer_new(int x, int y, int width, int height){
  //Window* w = malloc(sizeof *w);
  Window* frame = malloc(sizeof *frame);
  //Window_init(w, x, y, width, height);
  Window* w = Frame_init(frame, x, y, width, height);
  LOG_INFO("FileExplorer_new: %d %d %d %d %d", x, y, Window_get_absolute_x(w), Window_get_absolute_y(w), w->x);

  //w->draw = FileExplorer_draw;

  int j = 0;

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
  widget_width = 12; Window_add_widget(w, x_offset, j, widget_width, 1, " 📄 New File", BLACK, WHITE_BG); x_offset += widget_width;
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
  widget_width = 14; Window_add_widget(w, x_offset, j, widget_width, 1, " jordicolomer x", BLACK, WHITE_BG); x_offset += widget_width;
  widget_width = 1;  Window_add_widget(w, x_offset, j, -1, 1, " + ", BLACK, WHITE_BG); x_offset += widget_width;
  //Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
  j++;

  // address bar
  Window_add_widget(w, 0, j, -1, 1, " 📁 /Users/jordicolomer", BLACK, BRIGHT_BLUE_BG);
  j++;

  // favorites
  int start_j = j;
  int fav_width = 22;
  Window_add_widget(w, 0, j++, fav_width, 1, " Favorites", 255, 240);
  Window_add_widget(w, 0, j++, fav_width, 1, " 🏠 Home", 255, 247);
  Window_add_widget(w, 0, j++, fav_width, 1, " 📥 Downloads", 255, 247);
  Window_add_widget(w, 0, j++, fav_width, 1, " 📄 Documents", 255, 247);
  Window_add_widget(w, 0, j++, fav_width, 1, " 📷 Pictures", 255, 247);
  Window_add_widget(w, 0, j++, fav_width, 1, " 🎵 Music", 255, 247);
  Window_add_widget(w, 0, j++, fav_width, 1, " 🎬 Movies", 255, 247);
  Window_add_widget(w, 0, j++, fav_width, 1, "", 255, 247);
  Window_add_widget(w, 0, j++, fav_width, 1, " Locations", 255, 240);
  Window_add_widget(w, 0, j++, fav_width, 1, " 💻 Root", 255, 247);
  Window_add_widget(w, 0, j++, fav_width, 1, " 👥 Users", 255, 247);
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


  return frame;
}

Window* FileExplorer_test(int x, int y, int width, int height){
  Window* frame = malloc(sizeof *frame);
  Window* w = Frame_init(frame, x, y, width, height);
  return frame;
}

// TERMINAL

void init(){
  root = malloc(sizeof *root);
  Window_init(root, 0, 0, 200, 200);
  
  Window* w1 = FileExplorer_new(20, 10, 80, 20);
  w1->parent = root;
  Window_append(root, w1);
  
  Window* w2 = FileExplorer_new(30, 15, 80, 30);
  w2->parent = root;
  Window_append(root, w2);
  
  Window* w3 = FileExplorer_new(5, 5, 80, 30);
  w3->parent = root;
  Window_append(root, w3);
  
  /*Window* w2 = FileExplorer_test(100, 30, 80, 40);
  w2->parent = root;
  Window_append(root, w2);*/
  
  dragging = w1;
}


void repaint(){
  LOG_INFO("repaint");
  clear_screen();
  hide_cursor();
  
  // redraw everything
  Buffer_clear(&main_buf);
  root->draw(root, 0, 0);

#ifdef USE_BUFFER
	Buffer_print_to_screen(&main_buf);
#endif
}

void on_drag(int x, int y){
  LOG_INFO("on_drag x: %d y: %d", x, y);	
  if (dragging != NULL){
	LOG_INFO("dragging");	
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
  LOG_INFO("on_mouse_down: %d %d", x, y);
  Widget* wg = Window_find_widget(root, x, y);
  /*Widget* current = wg;
  LOG_INFO("Window_find_widget current: %p", (void*)current);
  while (current!=NULL){
	current = current->parent;
	}*/
  //if (wg != NULL) LOG_INFO("Window_find_widget parent: %p", wg->parent);

  if (wg != NULL){
	LOG_INFO("wg->on_mouse_down");
	if (wg->on_mouse_down != NULL) wg->on_mouse_down(wg, x, y);
	//dragging = wg;
  }
}

void on_mouse_up(){
  dragging = NULL;
  resizing = NULL;
}

//#include <locale.h>
int start() {
  log_init("app.log");

  //setlocale(LC_ALL, "");
  enable_raw_mode();
  enable_mouse();
  init();

  int rows; int cols; get_terminal_size(&rows, &cols);
  LOG_INFO("get_terminal_size: %d %d", rows, cols);
  Buffer_init(&main_buf, cols, rows);
  
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
		if (y == 1) continue;
		LOG_INFO("sscanf %d %d %d %c", btn, x, y, type);		
		if (dragging == 0 && btn == 0 && type == 'M') { //click
		  dragging = 1;
		  on_mouse_down(x, y);
		}

		if (type == 'm') { //release
		  dragging = 0;
		  on_mouse_up();
		}
			  
		if (dragging == 1){
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

/*
int test_buffer(){  
  Buffer buf;
  Buffer_init(&buf, 20, 20);
  Buffer_print(&buf, 0, 0, 10, "🔪hello", RED_BG, BLACK);
  Buffer_print(&buf, 1, 0, 10, "🔪hi", BLUE_BG, BLACK);
  Buffer_print(&buf, 3, 3, 11, "hello world", WHITE_BG, BLACK);
  Buffer_print(&buf, 0, 4, 10, "hello", GREEN_BG, BLACK);
  Buffer_print_to_screen(&buf);
}
*/

#include <signal.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
void write_stacktrace_to_fd(int fd)
{
    void *array[50];
    int size = backtrace(array, 50);

    backtrace_symbols_fd(array, size, fd);
}

void crash_handler(int sig)
{
    int fd = open("crash.log",
                  O_WRONLY | O_CREAT | O_APPEND,
                  0644);

    if (fd >= 0) {
        dprintf(fd, "\n--- Crash detected (signal %d) ---\n", sig);
        write_stacktrace_to_fd(fd);
        close(fd);
    }

    _exit(1);
}
void setup_crash_handler()
{
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE,  crash_handler);
}

#include <assert.h>

int test_windows1(){
  int grid[2][2] = {
    {1, 1},
    {1, 2}
  };
  log_init("app.log");
  Window* w1 = malloc(sizeof *w1);
  Window_init(w1, 0, 0, 2, 2);

  Window* w2 = malloc(sizeof *w2);
  Window_init(w2, 1, 1, 1, 1);
  Window_append(w1, w2);

  for (int i = 0; i <= 3; i++) {
	for (int j = 0; j <= 3; j++) {
	  Window* found = Window_find_widget(w1, i, j);
	  if (found){
		int idx = 0;
		if (found == w1) idx = 1;
		if (found == w2) idx = 2;
		assert(idx == grid[j][i]);
		printf("found: %d %d %d %d\n", i, j, idx, grid[j][i]);
	  }
	}
  }
}

int test_windows2(){
  int grid[5][5] = {
    {2, 2, 2, 1, 1},
    {2, 3, 2, 1, 1},
    {2, 2, 2, 1, 1},
    {1, 1, 4, 4, 1},
    {1, 1, 4, 4, 1},
  };
  log_init("app.log");
  Window* w1 = malloc(sizeof *w1);
  Window_init(w1, 0, 0, 5, 5);

  Window* w2 = malloc(sizeof *w2);
  Window_init(w2, 0, 0, 3, 3);
  Window_append(w1, w2);

  Window* w3 = malloc(sizeof *w3);
  Window_init(w3, 1, 1, 1, 1);
  Window_append(w2, w3);

  Window* w4 = malloc(sizeof *w4);
  Window_init(w4, 2, 3, 2, 2);
  Window_append(w1, w4);

  for (int i = 0; i < 5; i++) {
	for (int j = 0; j < 5; j++) {
	  Window* found = Window_find_widget(w1, i, j);
	  if (found){
		int idx = 0;
		if (found == w1) idx = 1;
		if (found == w2) idx = 2;
		if (found == w3) idx = 3;
		if (found == w4) idx = 4;
		assert(idx == grid[j][i]);
		printf("found: %d %d %d %d\n", i, j, idx, grid[j][i]);
		//printf("found: %d %d %d\n", i, j, idx);
	  }
	}
  }
}

int main() {
  setup_crash_handler();
  start();
  //test_buffer();
  //test_windows2();
}
