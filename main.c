#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "ansi_term.c"
#include "logger.c"


//#define USE_BUFFER

#define WINDOW_BAR_COLOR 20


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
typedef struct Geometry {
  int x;
  int y;
  int width;
  int height;
} Geometry;

typedef struct Window {
  char * id;
  struct Window* parent;
  struct Window* head;
  struct Window* tail;
  struct Window* next;
  struct Window* prev;
  //int x;
  //int y;
  int left;
  int right;
  int top;
  int bottom;
  int width;
  int height;
  //void (*draw)(struct Window*, int, int, int);
  void (*draw)(struct Window*, Geometry, int);
  void (*on_mouse_down) (struct Window* wg, int x, int y);  
  void (*on_hover) (struct Window* wg, int x, int y);  
  void (*undo_on_hover) (struct Window* wg, int x, int y);  
  void* data;
  void* data2;
  int hidden;
  int shift;
} Window;

Window* root = NULL;

Window* dragging = NULL;
Window* resizing = NULL;
Window* focused = NULL;
Window* hovering = NULL;
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

/*
int Window_get_width(Window* wg){
  if (wg->width >= 0){
	return wg->width;
  }
  return wg->parent->width - wg->left;
}

int Window_get_x(Window* wg){
  //LOG_INFO("Window_get_x");
  if (wg->left >= 0){
	//LOG_INFO("if");
	return wg->left;
  }
  //LOG_INFO("else");
  return wg->parent->width + wg->left;
}

int Window_get_y(Window* wg){
  if (wg-> >= 0){
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
*/


//void Window_draw(struct Window* w, int bias_x, int bias_y, int hasFocus){
void Window_draw(struct Window* w, Geometry geo, int hasFocus){
  //LOG_INFO("Window_draw %s w:%p geo.x:%d, geo.y:%d, geo.width:%d, geo.height:%d", w->id, w, geo.x, geo.y, geo.width, geo.height);
  if (focused == w) hasFocus = 1;

  Window* current = w->head;
  while (current != NULL) {

	// geo.width = w->left + w->width + w->right

	int left = current->left;
	int right = current->right;
	int width = current->width;

	if (left == -1) left = geo.width - current->right - current->width;
	if (right == -1) right = geo.width - current->left - current->width;
	if (width == -1) width = geo.width - current->left - current->right;

	// geo.height = w->top + w->height + w->bottom

	int top = current->top + w->shift;
	int bottom = current->bottom;
	int height = current->height;
	
	if (top == -1) top = geo.height - current->height - current->bottom;
	if (bottom == -1) bottom = geo.height - current->top - current->height;
	if (height == -1) height = geo.height - current->top - current->bottom;

	
	Geometry rect = {geo.x + left, geo.y + top, width, height};
	//LOG_INFO("Window_draw loop %s %p orig css left:%d right:%d width:%d top:%d bottom:%d height:%d", current->id, current, current->left, current->right, current->width, current->top, current->bottom, current->height);
	//LOG_INFO("Window_draw loop %s %p comp css left:%d right:%d width:%d top:%d bottom:%d height:%d", current->id, current, left, right, width, top, bottom, height);
	//LOG_INFO("Window_draw loop %s %p rect     rect.x:%d, rect.y:%d, rect.width:%d, rect.height:%d", current->id, current, rect.x, rect.y, rect.width, rect.height);
	//LOG_INFO("parent %s", w->id);
	int skip = 0;
	//if (top < 0) skip = 1;
	if (geo.height < top) skip = 1;
	//if (geo.height < bottom) skip = 1;
	if (height == 1 && top < 0) skip = 1;
	//if ((!(height == 1 && top < 0)) && top < geo.height)
	if (! skip) current->draw(current, rect, hasFocus || current == dragging);
	current = current->next;
  }
  //printf("\033[0m");
  //fflush(stdout);
}

Window* Window_init(Window* w, int left, int right, int top, int bottom, int width, int height){
  //Window* w = malloc(sizeof *w);
  w->head = NULL;
  w->tail = NULL;
  w->next = NULL;
  w->prev = NULL;
  
  w->left = left;
  w->right = right;
  w->top = top;
  w->bottom = bottom;
  //w->x = x;
  //w->y = y;
  w->width = width;
  w->height = height;
  //w->draw = draw;
  w->draw = Window_draw;
  w->on_hover = NULL;
  w->undo_on_hover = NULL;
  w->parent = NULL;
  w->hidden = 0;
  //w->shift = 0;


  return w;
}

/*
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


*/
int Geometry_in_bounds(Geometry geo, int x, int y){
  if (geo.x <= x && x < geo.x + geo.width &&
	  geo.y <= y && y < geo.y + geo.height) {
	return 1;
  }
  return 0;
}

Window* Window_find_widget(struct Window* this, Geometry geo, int x, int y){
  //Window* current = w->head;
  if (!this) return NULL;
  Window* ret = NULL;
  //LOG_INFO(" Window_find_widget %p %d %d %d %d %d %d", this, geo.x, geo.y, geo.width, geo.height, x, y);
  if (Geometry_in_bounds(geo, x, y)){
	//LOG_INFO("in bounds");
	ret = this;
  }
  Window* current = this->tail;
  while (current != NULL) {
	int left = current->left;
	int right = current->right;
	int width = current->width;

	if (left == -1) left = geo.width - current->right - current->width;
	if (right == -1) right = geo.width - current->left - current->width;
	if (width == -1) width = geo.width - current->left - current->right;

	int top = current->top;
	int bottom = current->bottom;
	int height = current->height;
	
	if (top == -1) top = geo.height - current->height - current->bottom;
	if (bottom == -1) bottom = geo.height - current->top - current->height;
	if (height == -1) height = geo.height - current->top - current->bottom;
	
	Geometry rect = {geo.x + left, geo.y + top, width, height};
	
	Window* found = Window_find_widget(current, rect, x, y);
	if (found != NULL) {
		return found;
	}
	
	current = current->prev;
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
  char * id;
  // for now we copy from Window
  struct Window* parent;
  struct Window* head;
  struct Window* tail;
  struct Window* next;
  struct Window* prev;
  //int x;
  //int y;
  int left;
  int right;
  int top;
  int bottom;
  int width;
  int height;
  //void (*draw)(struct Window*, int, int, int);
  void (*draw)(struct Window*, Geometry, int);
  void (*on_mouse_down) (struct Widget* wg, int x, int y);  
  void (*on_hover) (struct Window* wg, int x, int y);  
  void (*undo_on_hover) (struct Window* wg, int x, int y);  
  void* data;
  void* data2;
  int hidden;
  int shift;
  // for now we copy from Window
  
  char * c;
  int fg;
  int bg;
  //int _fg;
  //int _bg;
} Widget;


void Widget_draw(struct Window* wg, Geometry geo, int hasFocus){
  Widget* current = (Widget*) wg;
  Window_draw((Window*)current, geo, hasFocus);
#ifdef USE_BUFFER
	Buffer_print(&main_buf, geo.y + wg_y, geo.x + wg_x, wg_width, current->c, current->fg, current->bg);
#else

	if (wg->hidden) return;

	// check if visible
	int visible = 1;
	Window* cursor = wg;
	while (cursor->parent != root){
	  cursor = cursor->parent;
	}
	cursor = cursor->next;
	while (cursor != NULL) {
	  if (cursor->left < geo.x && geo.x + geo.width < cursor->left + cursor->width &&
		  cursor->top < geo.y && geo.y + geo.height < cursor->top + cursor->height) {
		//LOG_INFO("skipping Widget_draw");
		return;
	  }
		//visible = 0;
	  cursor = cursor->next;
	}
	//if (! visible) return;

	
	int fg = current->fg;
	int bg = current->bg;
	if (bg >= 232+4 && ! hasFocus) bg -= 4;
	if (bg == WINDOW_BAR_COLOR && ! hasFocus) bg = 243;
	Buffer_print_raw(&main_buf, geo.y, geo.x, geo.width, current->c, fg, bg);
	//fprintf(stdout, "\033[0m");
	//fflush(stdout);
#endif
}


Widget* Window_add_widget(Window* w, int left, int right, int top, int bottom, int width, int height, char * c, int fg, int bg){
  Widget* wg = malloc(sizeof *wg);
  Window_init(wg, left, right, top, bottom, width, height);
  wg->draw = Widget_draw;
  wg->parent = w;
  wg->c = c;
  wg->id = c;
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
  focused = wg->parent;
  Window_bring_to_bottom(dragging);
  dragging_offset_x = x - wg->parent->left;
  dragging_offset_y = y - wg->parent->top;
}


void Window_add_window_bar(struct Window* w){
  Widget* wg = Window_add_widget(w, 0, 0, 0, -1, -1, 1, "", 255, WINDOW_BAR_COLOR);
  wg->on_mouse_down = on_mouse_down_window_bar;
  
  Widget* close =    Window_add_widget(w, -1, 2, 0, -1, 1, 1, "X", 255, WINDOW_BAR_COLOR);
  Widget* maximize = Window_add_widget(w, -1, 4, 0, -1, 1, 1, "□", 255, WINDOW_BAR_COLOR);
  Widget* minimize = Window_add_widget(w, -1, 6, 0, -1, 1, 1, "-", 255, WINDOW_BAR_COLOR);
}

void Widget_on_resize(Widget* wg, int x, int y){
  LOG_INFO("Widget_on_resize");

  resizing = wg->parent;
  dragging_offset_x = x - wg->parent->width;
  dragging_offset_y = y - wg->parent->height;
}

Window* Frame_init(Window* w, int left, int right, int top, int bottom, int width, int height){
  Window_init(w, left, right, top, bottom, width, height);
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

  Window* child = malloc(sizeof *child);
  //Window_init(child, 1, 1, width-2, height-2);
  Window_init(child, 0, 0, 1, 0, -1, -1);
  Window_append(w, child);
  child->id = "child";
  
  Widget* resize_grip = Window_add_widget(w, -1, 0, -1, 0, 1, 1, "▟", 250, 255);
  resize_grip->on_mouse_down = Widget_on_resize;

  return child;
}

/* file_manager.c */

typedef struct Slider_data{
  Window* slider_grip;
  Window* child;
  int height;
} Slider_data;

void Slider_hover(Window* wg, int x, int y){
  Window* slider_grip = ((Slider_data*) wg->data)->slider_grip;
  slider_grip->hidden = 0;
  //LOG_INFO("Slider_hover");
}

void Slider_grip_hover(Window* wg, int x, int y){
  wg->hidden = 0;
}

void Slider_grip_undo_hover(Window* wg, int x, int y){
  wg->hidden = 1;
}

void Slider_undo_hover(Window* wg, int x, int y){
  Window* slider_grip = ((Slider_data*) wg->data)->slider_grip;
  slider_grip->hidden = 1;
  //LOG_INFO("Slider_hover");
}

void Slider_on_mouse_down(Window* wg, int x, int y){
  Slider_data * slider_data = (Slider_data*) wg->data;
  Window* child = slider_data->child;
  child->shift -= slider_data->height;
}

void Slider_grip_draw(struct Window* w, Geometry geo, int hasFocus){
  Slider_data * slider_data = (Slider_data*) w->parent->data;
  Window* child = slider_data->child;
  child->shift = -w->top;
  Widget_draw(w, geo, hasFocus);
}

void on_mouse_down_slider_grip(Window* wg, int x, int y){
  /*
  Slider_data * slider_data = (Slider_data*) wg->parent->data;
  Window* child = slider_data->child;
  child->shift -= 1;
  */
  
  dragging = wg;
  dragging_offset_x = x - wg->parent->left;
  //dragging_offset_y = y - wg->parent->top;
  dragging_offset_y = y - wg->parent->top - wg->top;
}


Window* FileExplorer_new(int left, int right, int top, int bottom, int width, int height){
  //Window* w = malloc(sizeof *w);
  Window* frame = malloc(sizeof *frame);
  //Window_init(w, x, y, width, height);
  Window* w = Frame_init(frame, left, right, top, bottom, width, height);
  //LOG_INFO("FileExplorer_new: %d %d %d %d %d", x, y, Window_get_absolute_x(w), Window_get_absolute_y(w), w->x);

  //w->draw = FileExplorer_draw;

  int j = 0;

  int widget_width;
  

  // menubar
  int x_offset = 0;
  widget_width = 6; Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " File", 232, 253); x_offset += widget_width;
  widget_width = 6; Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " Edit", 232, 253); x_offset += widget_width;
  widget_width = 6;  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " View", 232, 253); x_offset += widget_width;
  widget_width = 6;  Window_add_widget(w, x_offset, 0, j, -1, -1, 1, " Help", 232, 253); x_offset += widget_width;
  //Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
  j++;

  // toolbar
  x_offset = 0;
  widget_width = 12; Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " 📄 New File", 232, 254); x_offset += widget_width;
  widget_width = 12; Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "📁 New Dir", 232, 254); x_offset += widget_width;
  widget_width = 8;  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "📋 Copy", 232, 254); x_offset += widget_width;
  widget_width = 8;  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "🔪 Cut", 232, 254); x_offset += widget_width;
  widget_width = 10; Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "📌 Paste", 232, 254); x_offset += widget_width;
  widget_width = 10; Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "🔤 Rename", 232, 254); x_offset += widget_width;
  widget_width = 10; Window_add_widget(w, x_offset, 0, j, -1, -1, 1, "❌ Delete", 232, 254); x_offset += widget_width;
  //Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
  j++;

  //Widget* Window_add_widget(Window* w, int left, int right, int top, int bottom, int width, int height, char * c, int fg, int bg){
  // tabs
  x_offset = 0;
  widget_width = 14; Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " jordicolomer x", 232, 255); x_offset += widget_width;
  widget_width = 1;  Window_add_widget(w, x_offset, 0, j, 1, -1, 1, " + ", 232, 255); x_offset += widget_width;
  //Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
  j++;

  // address bar
  Window_add_widget(w, 0, 0, j, -1, -1, 1, " 📁 /Users/jordicolomer", 232, 255);
  j++;

  // favorites
  int start_j = j;
  int fav_width = 22;
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " Favorites", 255, 245);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🏠 Home", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📥 Downloads", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📄 Documents", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📷 Pictures", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🎵 Music", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🎬 Movies", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, "", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " Locations", 255, 245);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 💻 Root", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 👥 Users", 232, 254);
  while(j <= 200) Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, "", 232, 255);

  // list files
  Window* fm_slider = malloc(sizeof *fm_slider);
  Window_init(fm_slider, fav_width, 0, 4, 0, -1, -1);
  Window_append(w, fm_slider);
  
  Window* fm = malloc(sizeof *fm);
  Window_init(fm, 0, 0, 0, 0, -1, -1);
  Window_append(fm_slider, fm);
  
  j = 0;
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
	//Window_add_widget(w, fav_width, 0, j++, -1, -1, 1, str, 232, 255);
	Window_add_widget(fm, 0, 0, j++, -1, -1, 1, str, 232, 255);
	//if (height < j) break;
	//mvwprintw(win, x++, 1, "%s %s", icon, entry->d_name);
  }

  closedir(dir);

  Window* slider = malloc(sizeof *slider);
  Window_init(slider, -1, 0, 0, 0, 2, -1);
  Window_append(fm_slider, slider);
  slider->on_hover = Slider_hover;
  slider->undo_on_hover = Slider_undo_hover;
  slider->on_mouse_down = Slider_on_mouse_down;

  Slider_data* slider_data = (Slider_data*) malloc(sizeof(Slider_data));
  slider->data = slider_data;
  slider_data->child = fm;
  slider_data->height = height-4;
    
  Window* slider_grip = Window_add_widget(slider, -1, 0, 0, -1, 2, 1, "░░", 232, 255);
  slider_grip->on_mouse_down  = on_mouse_down_slider_grip;
  slider_grip->hidden = 1;
  slider_grip->on_hover = Slider_grip_hover;
  slider_grip->undo_on_hover = Slider_grip_undo_hover;
  slider_grip->draw = Slider_grip_draw;
  slider_data->slider_grip = slider_grip;

  return frame;
}

Window* FileExplorer_test(int x, int y, int width, int height){
  Window* frame = malloc(sizeof *frame);
  Window* w = Frame_init(frame, x, -1, y, -1, width, height);
  return frame;
}

// TERMINAL

void init(){
  int rows; int cols; get_terminal_size(&rows, &cols);
  LOG_INFO("get_terminal_size: %d %d", rows, cols);
  Buffer_init(&main_buf, cols, rows);

  root = malloc(sizeof *root);
   
  Window_init(root, 0, -1, 0, -1, cols, rows);
  
  Window* w1 = FileExplorer_new(20, -1, 10, -1, 80, 20);
  w1->parent = root;
  w1->id = "FileExplorer1";
  Window_append(root, w1);
  
  Window* w2 = FileExplorer_new(30, -1, 15, -1, 80, 30);
  w2->parent = root;
  w2->id = "FileExplorer2";
  Window_append(root, w2);
  
  Window* w3 = FileExplorer_new(5, -1, 5, -1, 80, 30);
  w3->parent = root;
  w3->id = "FileExplorer3";
  Window_append(root, w3);
  
  /*Window* w2 = FileExplorer_test(100, 30, 80, 40);
  w2->parent = root;
  Window_append(root, w2);*/
  
  //dragging = w1;
}


void repaint(){
  LOG_INFO("repaint");
  clear_screen();
  hide_cursor();
  
  // redraw everything
#ifdef USE_BUFFER
  Buffer_clear(&main_buf);
#endif
  
  Geometry rect = {0, 0, root->width, root->height};
  root->draw(root, rect, 0);
  fprintf(stdout, "\033[0m");
  fflush(stdout);

#ifdef USE_BUFFER
	Buffer_print_to_screen(&main_buf);
#endif
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

int min(int a, int b) {
    return (a < b) ? a : b;
}

void on_drag(int x, int y){
  //LOG_INFO("on_drag x: %d y: %d", x, y);	
  if (dragging != NULL){
	LOG_INFO("dragging");
	dragging->left = min(max(0, x - dragging_offset_x), dragging->parent->width-2);
	dragging->top = max(0, y - dragging_offset_y);
	repaint();
  }
  else if (resizing != NULL){
	resizing->width = x - dragging_offset_x;
	resizing->height = y - dragging_offset_y;
	LOG_INFO("new dimensions width: %d height: %d", resizing->width, resizing->height);	
	repaint();
  } else {
	Geometry rect = {0, 0, root->width, root->height};
	Widget* wg = Window_find_widget(root, rect, x, y);
	if (wg != NULL && wg->on_hover != NULL){
	  hovering = wg;
	  wg->on_hover(wg, x, y);
	  repaint();
	} else if (hovering != NULL && hovering->undo_on_hover != NULL) {
	  hovering->undo_on_hover(hovering, x, y);
	  repaint();
	  hovering = NULL;
	}
  }
}

void on_mouse_down(int x, int y){
  LOG_INFO("on_mouse_down: %d %d", x, y);
  Geometry rect = {0, 0, root->width, root->height};
  Widget* wg = Window_find_widget(root, rect, x, y);
  LOG_INFO("Window_find_widget: %p", (void*)wg);
  /*Widget* current = wg;
  while (current!=NULL){
	current = current->parent;
	}*/
  //if (wg != NULL) LOG_INFO("Window_find_widget parent: %p", wg->parent);

  if (wg != NULL){
	LOG_INFO("wg id:%s wg->left:%d wg->right:%d wg->top:%d wg->bottom:%d wg->width:%d wg->height:%d", wg->id, wg->left, wg->right, wg->top, wg->bottom, wg->width, wg->height);
	if (wg->on_mouse_down != NULL) {
	  LOG_INFO("wg->on_mouse_down");
	  wg->on_mouse_down(wg, x, y);
	}
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

  
  int dragging = 0;
  repaint();

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
		//LOG_INFO("sscanf %d %d %d %c", btn, x, y, type);		
		if (dragging == 0 && btn == 0 && type == 'M') { //click
		  dragging = 1;
		  on_mouse_down(x, y);
		}

		if (type == 'm') { //release
		  dragging = 0;
		  on_mouse_up();
		}
			  
		//if (dragging == 1){
		  on_drag(x, y);
		  //}
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
  Window_init(w1, 0, -1, 0, -1, 2, 2);

  Window* w2 = malloc(sizeof *w2);
  Window_init(w2, 1, -1, 1, -1, 1, 1);
  Window_append(w1, w2);

  for (int i = 0; i <= 3; i++) {
	for (int j = 0; j <= 3; j++) {
	  Geometry rect = {0, 0, w1->width, w1->height};
	  Window* found = Window_find_widget(w1, rect, i, j);
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
  Window_init(w1, 0, -1, 0, -1, 5, 5);

  Window* w2 = malloc(sizeof *w2);
  Window_init(w2, 0, -1, 0, -1, 3, 3);
  Window_append(w1, w2);

  Window* w3 = malloc(sizeof *w3);
  Window_init(w3, 1, -1, 1, -1, 1, 1);
  Window_append(w2, w3);

  Window* w4 = malloc(sizeof *w4);
  Window_init(w4, 2, -1, 3, -1, 2, 2);
  Window_append(w1, w4);

  for (int i = 0; i < 5; i++) {
	for (int j = 0; j < 5; j++) {
	  Geometry rect = {0, 0, w1->width, w1->height};
	  Window* found = Window_find_widget(w1, rect, i, j);
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
